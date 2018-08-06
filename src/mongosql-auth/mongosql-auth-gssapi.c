#include <string.h>
#include <stdio.h>
#include "mongosql-auth.h"
#include "mongosql-auth-conversation.h"
#include "mongosql-auth-gssapi.h"

uint8_t _mongosql_auth_sasl_init(mongosql_auth_sasl_client* sasl,
                                 char* username,
                                 char* password,
                                 char* target_spn,
                                 char** errmsg)
{
    int err;
    sasl->state = SASL_START;
    mongosql_auth_log("%s","    Initializing GSSAPI client");

    if (errmsg) {
        *errmsg = NULL;
    }
    err = mongosql_auth_gssapi_client_init(&sasl->client, username, password, target_spn);
    if (err == GSSAPI_OK) {
        return SASL_OK;
    }
    mongosql_auth_gssapi_log_error(&sasl->client, "initializing client", errmsg);
    return SASL_ERR;
}

uint8_t _mongosql_auth_sasl_step(mongosql_auth_sasl_client* sasl,
                                uint8_t *inbuf,
                                size_t inbuflen,
                                uint8_t **outbuf,
                                size_t *outbuflen,
                                char **error)
{
    int status;
    char* username;
    uint8_t *msg;
    size_t userlen, msglen;
    const uint8_t qop[] = {1, 0, 0, 0};

    if (error) {
        *error = NULL;
    }

    mongosql_auth_log("%s: %d","    Entering GSSAPI auth step. SASL state: ", sasl->state);
    switch (sasl->state) {
    case SASL_START:
        // Initiate the GSSAPI context with the server.
        status = mongosql_auth_gssapi_client_negotiate(&sasl->client,
                inbuf,
                inbuflen,
                outbuf,
                outbuflen);
        if (status == GSSAPI_OK) {
            sasl->state = SASL_CONTEXT_COMPLETE;
            mongosql_auth_log("%s","      Done initiating context");
            return SASL_OK;
        } else if (status == GSSAPI_CONTINUE) {
            mongosql_auth_log("%s","      Continue neededed for GSSAPI auth");
            return SASL_OK;
        } else {
            mongosql_auth_gssapi_log_error(&sasl->client, "negotiating with server", error);
            return SASL_ERR;
        }

    case SASL_CONTEXT_COMPLETE:
        // Get the client username from the credential.
        status = mongosql_auth_gssapi_client_username(&sasl->client, &username); 
        if (status != GSSAPI_OK) {
            mongosql_auth_gssapi_log_error(&sasl->client, "getting client display name", error);
            return SASL_ERR;
        }

        userlen = strlen(username);
        msglen = 4 + userlen;
        msg = malloc(msglen);
        memcpy(msg, qop, 4);
        memcpy(&msg[4], username, userlen);
        free(username);

        // Wrap the quality of protection message followed by the client username.
        status = mongosql_auth_gssapi_client_wrap_msg(&sasl->client,
                    msg,
                    msglen,
                    outbuf,
                    outbuflen);

        free(msg);
        if (status != GSSAPI_OK) {
            mongosql_auth_gssapi_log_error(&sasl->client, "wrapping quality of protection message", error);
            return SASL_ERR;
        }

        sasl->state = SASL_DONE;
        return SASL_OK;
    case SASL_DONE:
       mongosql_auth_log("%s: %d","      Invalid state in sasl client", sasl->state);
       return SASL_ERR; 
    }
    return SASL_ERR;
}
uint8_t _mongosql_auth_sasl_destroy(mongosql_auth_sasl_client* sasl)
{
    return (mongosql_auth_gssapi_client_destroy(&sasl->client) != GSSAPI_OK) ? SASL_ERR : SASL_OK;
}

OM_uint32 mongosql_copy_and_release_buffer(
    OM_uint32* minor_status,
    gss_buffer_desc* buffer,
    uint8_t** output,
    size_t* output_length
)
{
    OM_uint32 major_status;
    *output = malloc(buffer->length);
    *output_length = buffer->length;
    if (*output) {
        memcpy(*output, buffer->value, buffer->length);
    }

    major_status = gss_release_buffer(
        minor_status,   // minor_status
        buffer         // buffer
    );

    if (GSS_ERROR(major_status) && *output) {
        free(*output);
    }

    return major_status;
}

OM_uint32 mongosql_auth_gssapi_canonicalize_name(
    OM_uint32* minor_status,
    char *input_name,
    gss_OID input_name_type,
    gss_name_t *output_name
)
{
    OM_uint32 major_status;
    gss_name_t imported_name = GSS_C_NO_NAME;
    gss_buffer_desc input_name_buffer = GSS_C_EMPTY_BUFFER;

    input_name_buffer.value = input_name;
    input_name_buffer.length = strlen(input_name);
    major_status = gss_import_name(
        minor_status,       // minor_status
        &input_name_buffer, // input_name_buffer
        input_name_type,    // input_name_type
        &imported_name      // output_name
    );

    if (GSS_ERROR(major_status)) {
        return major_status;
    }

    major_status = gss_canonicalize_name(
        minor_status,           // minor_status
        imported_name,          // input_name
        (gss_OID)gss_mech_krb5, // mech_type
        output_name             // output_name
    );

    if (imported_name != GSS_C_NO_NAME) {
        major_status = gss_release_name(
            minor_status,   // minor_status
            &imported_name  // name
        );
    }

    return major_status;
}

OM_uint32 mongosql_auth_gssapi_display_name(
    OM_uint32 *minor_status,
    gss_cred_id_t cred,
    char** output_name
)
{
    OM_uint32 major_status;
    OM_uint32 ignore;
    gss_name_t name = GSS_C_NO_NAME;
    gss_buffer_desc name_buffer;

    major_status = gss_inquire_cred(
        minor_status,   // minor_status
        cred,           // cred_handle
        &name,          // name
        NULL,           // lifetime
        NULL,           // cred_usage
        NULL            // mechanisms
    );

    if (GSS_ERROR(major_status)) {
        return major_status;
    }

    major_status = gss_display_name(minor_status, name, &name_buffer, NULL);
    if (GSS_ERROR(major_status)) {

        // ignore result here because it would mask the error from display name
        gss_release_name(
            &ignore,    // minor_status
            &name       // name
        );

        return major_status;
    }

    major_status = gss_release_name(
        minor_status,   // minor_status
        &name);         // name

    if (GSS_ERROR(major_status)) {
        if (name_buffer.length) {
            // ignore result here because it would mask the error from release_name
            gss_release_buffer(
                &ignore,   // minor_status
                &name_buffer    // buffer
            );
        }
        return major_status;
    }

    if (name_buffer.length) {
        *output_name = malloc(name_buffer.length+1);
        memcpy(*output_name, name_buffer.value, name_buffer.length+1);

        major_status = gss_release_buffer(
            minor_status,   // minor_status
            &name_buffer    // buffer
        );

        if (GSS_ERROR(major_status) && *output_name) {
            free(*output_name);
        }
    }

    return major_status;
}

OM_uint32 mongosql_auth_gssapi_wrap_msg(
    OM_uint32 *minor_status,
    gss_ctx_id_t ctx,
    uint8_t* input,
    size_t input_length,
    uint8_t** output,
    size_t* output_length
)
{
    OM_uint32 major_status;
    gss_buffer_desc input_buffer = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc output_buffer = GSS_C_EMPTY_BUFFER;

    input_buffer.value = input;
    input_buffer.length = input_length;

    major_status = gss_wrap(
        minor_status,       // minor_status
        ctx,                // context_handle
        0,                  // conf_req_flag
        GSS_C_QOP_DEFAULT,  // qop_req
        &input_buffer,      // input_message_buffer
        NULL,               // conf_state
        &output_buffer      // output_message_buffer
    );

    if (GSS_ERROR(major_status)) {
        return major_status;
    }

    if (output_buffer.length) {
        major_status = mongosql_copy_and_release_buffer(
            minor_status,
            &output_buffer,
            output,
            output_length
        );
    }

    return major_status;
}

/**
 * Get the GSSAPI error message and log it to the standard output with a given prefix.
 *
 * If 'errmsg' is provided, allocate a buffer for returning the message description to the caller
 */
void mongosql_auth_gssapi_log_error(
    mongosql_auth_gssapi_client *client,
    const char *prefix,
    char **errmsg
) {
    char *tmp = NULL;
    int status = mongosql_auth_gssapi_error_desc(client->maj_stat, client->min_stat, &tmp);
    if (status == GSSAPI_OK) {
        mongosql_auth_log("      GSSAPI Error %s: %s (%d, %d)", prefix, tmp, client->maj_stat, client->min_stat);

        // If errmsg is provided,  pass the buffer up to the caller. Discard otherwise.
        if (errmsg) {
            *errmsg = tmp;
        } else {
            free(tmp);
        }
    } else {
        mongosql_auth_log("      GSSAPI Error %s, but could not get description: (%d, %d)", prefix, client->maj_stat, client->min_stat);
    }
}

int mongosql_auth_gssapi_error_desc(
    OM_uint32 maj_stat,
    OM_uint32 min_stat,
    char **desc
)
{
    OM_uint32 local_maj_stat, local_min_stat;
    OM_uint32 msg_ctx = 0;
    OM_uint32 stat = maj_stat;
    gss_buffer_desc desc_buffer;
    int stat_type = GSS_C_GSS_CODE;

    if (min_stat != 0) {
        stat = min_stat;
        stat_type = GSS_C_MECH_CODE;
    }

    // error codes are stored in a hierarchical fashion. This loop
    // will overwrite more general errors with more specific errors.
    do
    {
        if (*desc) {
            free(*desc);
        }

        local_maj_stat = gss_display_status(
            &local_min_stat,    // minor_status
            stat,               // status_value
            stat_type,          // status_type
            GSS_C_NO_OID,       // mech_type
            &msg_ctx,           // message_context
            &desc_buffer        // status_string
        );

        if (GSS_ERROR(local_maj_stat)) {
            return GSSAPI_ERROR;
        }

        *desc = malloc(desc_buffer.length+1);
        if (*desc) {
            memcpy(*desc, desc_buffer.value, desc_buffer.length+1);
        }

        local_maj_stat = gss_release_buffer(
            &local_min_stat,    // minor_status
            &desc_buffer        // buffer
        );

        if (GSS_ERROR(local_maj_stat)) {
            if (*desc) {
                free(*desc);
            }
            return GSSAPI_ERROR;
        }
    }
    while(msg_ctx != 0);

    return GSSAPI_OK;
}

int mongosql_auth_gssapi_client_init(
    mongosql_auth_gssapi_client *client,
    char* username,
    char* password,
    char* target_spn
)
{
    OM_uint32 major_status;
    OM_uint32 minor_status;
    gss_name_t client_name = GSS_C_NO_NAME;
    gss_buffer_desc password_buffer = GSS_C_EMPTY_BUFFER;

    client->cred = GSS_C_NO_CREDENTIAL;
    client->ctx = GSS_C_NO_CONTEXT;
    client->spn = GSS_C_NO_NAME;

    // Get a the canonicalized name for the user principal
    mongosql_auth_log("%s: %s","      Canonicalizing name for user", username);
    client->maj_stat = mongosql_auth_gssapi_canonicalize_name(
        &client->min_stat,
        username,
        GSS_C_NT_USER_NAME,
        &client_name
    );

    if (GSS_ERROR(client->maj_stat)) {
        return GSSAPI_ERROR;
    }

    // Get a canonicalized name for the service name principal
    mongosql_auth_log("%s: %s","      Canonicalizing name for SPN", target_spn);
    client->maj_stat = mongosql_auth_gssapi_canonicalize_name(
        &client->min_stat,
        target_spn,
        GSS_C_NT_HOSTBASED_SERVICE,
        &client->spn
    );

    if (GSS_ERROR(client->maj_stat)) {
        return GSSAPI_ERROR;
    }

    // Acquire credentials for the user principal
    if (password && strlen(password) > 0) {
        // Use the provided password
        password_buffer.value = password;
        password_buffer.length = strlen(password);
        mongosql_auth_log("%s","      Acquiring credentials with password");
        client->maj_stat = gss_acquire_cred_with_password(
            &client->min_stat,  // minor_status
            client_name,        // desired_name
            &password_buffer,   // password
            GSS_C_INDEFINITE,   // time_req
            GSS_C_NO_OID_SET,   // desired_mech (default)
            GSS_C_INITIATE,     // cred_user
            &client->cred,      // output_cred_handle
            NULL,               // actual_mechs
            NULL                // time_rec
        );
    } else {
        mongosql_auth_log("%s","      Acquiring credentials");
        client->maj_stat = gss_acquire_cred(
            &client->min_stat,  // minor_status
            client_name,        // desired_name
            GSS_C_INDEFINITE,   // time_req
            GSS_C_NO_OID_SET,   // desired_mech (default)
            GSS_C_INITIATE,     // cred_user
            &client->cred,      // output_cred_handle
            NULL,               // actual_mechs
            NULL                // time_rec
        );
    }

    // Release the user name. It is no longer needed because we might have the credentials.
    if (client_name != GSS_C_NO_NAME) {
        major_status = gss_release_name(
            &minor_status,  // minor_status
            &client_name    // name
        );

        // Only report this error if gss_acquire_cred didn't have an error
        if (GSS_ERROR(major_status) && !GSS_ERROR(client->maj_stat)) {
            client->maj_stat = major_status;
            client->min_stat = minor_status;
        }
    }

    if (GSS_ERROR(client->maj_stat)) {
        return GSSAPI_ERROR;
    }

    return GSSAPI_OK;
}

int mongosql_auth_gssapi_client_username(
    mongosql_auth_gssapi_client *client,
    char** username
)
{
    client->maj_stat = mongosql_auth_gssapi_display_name(
        &client->min_stat,
        client->cred,
        username
    );

    if (GSS_ERROR(client->maj_stat)) {
        return GSSAPI_ERROR;
    }
    return GSSAPI_OK;
}

int mongosql_auth_gssapi_client_negotiate(
    mongosql_auth_gssapi_client *client,
    uint8_t* input,
    size_t input_length,
    uint8_t** output,
    size_t* output_length
)
{
    gss_buffer_desc input_buffer = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc output_buffer = GSS_C_EMPTY_BUFFER;
    OM_uint32 major_status;
    OM_uint32 minor_status;

    if (input) {
        input_buffer.value = input;
        input_buffer.length = input_length;
    }

    mongosql_auth_log("%s","      Initiating GSS security context");
    client->maj_stat = gss_init_sec_context(
        &client->min_stat,          // minor_status
        client->cred,               // initiator_cred_handle
        &client->ctx,               // context_handle
        client->spn,                // target_name
        GSS_C_NO_OID,               // mech_type
        (GSS_C_MUTUAL_FLAG | GSS_C_INTEG_FLAG | GSS_C_DELEG_FLAG), // req_flags
        0,                          // time_req
        GSS_C_NO_CHANNEL_BINDINGS,  // input_chan_bindings
        &input_buffer,              // input_token
        NULL,                       // actual_mech_type
        &output_buffer,             // output_token
        NULL,                       // ret_flags
        NULL                        // time_rec
    );

    if (GSS_ERROR(client->maj_stat)) {
        return GSSAPI_ERROR;
    }

    if (output_buffer.length) {
        major_status = mongosql_copy_and_release_buffer(
            &minor_status,
            &output_buffer,
            output,
            output_length
        );

        if (GSS_ERROR(major_status)) {
            client->maj_stat = major_status;
            client->min_stat = minor_status;
            return GSSAPI_ERROR;
        }
    } else {
        *output_length = 0;
    }

    if (client->maj_stat == GSS_S_CONTINUE_NEEDED) {
        return GSSAPI_CONTINUE;
    }
    return GSSAPI_OK;
}

int mongosql_auth_gssapi_client_wrap_msg(
    mongosql_auth_gssapi_client *client,
    uint8_t* input,
    size_t input_length,
    uint8_t** output,
    size_t* output_length
)
{
    client->maj_stat = mongosql_auth_gssapi_wrap_msg(
        &client->min_stat,
        client->ctx,
        input,
        input_length,
        output,
        output_length
    );

    if (GSS_ERROR(client->maj_stat)) {
        return GSSAPI_ERROR;
    }
    return GSSAPI_OK;
}

int mongosql_auth_gssapi_client_destroy(
    mongosql_auth_gssapi_client *client
)
{
    int result = GSSAPI_OK;
    OM_uint32 major_status;
    OM_uint32 minor_status;
    if (client->ctx != GSS_C_NO_CONTEXT) {
        major_status = gss_delete_sec_context(
            &minor_status,      // minor_status
            &client->ctx,       // context_handle
            GSS_C_NO_BUFFER     // output_token
        );

        if (GSS_ERROR(major_status)) {
            result = GSSAPI_ERROR;
        }
    }

    if (client->spn != GSS_C_NO_NAME) {
        major_status = gss_release_name(
            &minor_status,  // minor_status
            &client->spn    // name
        );
        if (GSS_ERROR(major_status)) {
            result = GSSAPI_ERROR;
        }
    }

    if (client->cred != GSS_C_NO_CREDENTIAL) {
        major_status = gss_release_cred(
            &minor_status,
            &client->cred
        );
        if (GSS_ERROR(major_status)) {
            result = GSSAPI_ERROR;
        }
    }
    return result;
}

