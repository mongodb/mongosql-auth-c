#include <string.h>
#include <stdio.h>
#include "mongosql-auth-conversation.h"
#include "mongosql-auth-sspi.h"

#define _SSPI_NOT_SUPPORTED "SSPI is not implemented yet."

uint8_t _mongosql_auth_sasl_init(mongosql_auth_sasl_client* sasl,
                                 char* username,
                                 char* password,
                                 char* target_spn,
                                 char** errmsg)
{
    int err;
    sasl->state = SASL_START;

    if (errmsg) {
        *errmsg = NULL;
    }

    err = sspi_init();
    if (err != SSPI_OK) {
        return SASL_ERR;
    }

    err = mongosql_auth_sspi_client_init(&sasl->client, username, password, target_spn);
    if (err != SSPI_OK) {
        return SASL_ERR;
    }

    return SASL_OK;
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

    MONGOSQL_AUTH_LOG("%s: %d","    Entering SSPI auth step. SASL state: ", sasl->state);
    switch (sasl->state) {
    case SASL_START:
        // Initiate the SSPI context with the server.
        status = mongosql_auth_sspi_client_negotiate(&sasl->client,
                inbuf,
                inbuflen,
                outbuf,
                outbuflen);
        if (status == SSPI_OK) {
            sasl->state = SASL_CONTEXT_COMPLETE;
            MONGOSQL_AUTH_LOG("%s","      Done initiating context");
            return SASL_OK;
        } else if (status == SSPI_CONTINUE) {
            MONGOSQL_AUTH_LOG("%s","      Continue needed for SSPI auth");
            return SASL_OK;
        } else {
            return SASL_ERR;
        }

    case SASL_CONTEXT_COMPLETE:
        // Get the client username from the credential.
        status = mongosql_auth_sspi_client_username(&sasl->client, &username);
        if (status != SSPI_OK) {
            return SASL_ERR;
        }

        userlen = strlen(username);
        msglen = 4 + userlen;
        msg = malloc(msglen);
        memcpy(msg, qop, 4);
        memcpy(&msg[4], username, userlen);
        free(username);

        // Wrap the quality of protection message followed by the client username.
        status = mongosql_auth_sspi_client_wrap_msg(&sasl->client,
                    msg,
                    msglen,
                    outbuf,
                    outbuflen);

        free(msg);
        if (status != SSPI_OK) {
            return SASL_ERR;
        }

        sasl->state = SASL_DONE;
        return SASL_OK;
    case SASL_DONE:
       MONGOSQL_AUTH_LOG("%s: %d","      Invalid state in sasl client", sasl->state);
       return SASL_ERR;
    }
    return SASL_ERR;
}

uint8_t _mongosql_auth_sasl_destroy(mongosql_auth_sasl_client* sasl)
{
    int err;
    err = mongosql_auth_sspi_client_destroy(&sasl->client);
    if (err != SSPI_OK) {
        return SASL_ERR;
    }
    return SASL_OK;
}

static HINSTANCE sspi_secur32_dll = NULL;
static PSecurityFunctionTable sspi_functions = NULL;
static const LPSTR SSPI_PACKAGE_NAME = "kerberos";

int sspi_init(
)
{
    sspi_secur32_dll = LoadLibrary("secur32.dll");
    if (!sspi_secur32_dll) {
        return GetLastError();
    }

    INIT_SECURITY_INTERFACE init_security_interface = (INIT_SECURITY_INTERFACE)GetProcAddress(sspi_secur32_dll, SECURITY_ENTRYPOINT);
    if (!init_security_interface) {
        return -1;
    }

    sspi_functions = (*init_security_interface)();
    if (!sspi_functions) {
        return -2;
    }

    return SSPI_OK;
}

int mongosql_auth_sspi_client_init(
    mongosql_auth_sspi_client *client,
    char* username,
    char* password,
    char* target_spn
)
{
    char* principal_name;
    PVOID auth_data;

    TimeStamp timestamp;
    SEC_WINNT_AUTH_IDENTITY auth_identity;

    auth_data = NULL;
    principal_name = NULL;

    if (target_spn) {
      client->spn = malloc(strlen(target_spn)+1);
      memcpy(client->spn, target_spn, strlen(target_spn)+1);
    }

    if (!username) {
        // If no username was provided, auth with the default credentials.
        // So we don't set principal_name or auth_data here.
        MONGOSQL_AUTH_LOG("%s","      Acquiring default credentials");
    } else if (!password) {
        // If a username was provided but no password,
        // pass in the username as principal_name but no auth_data.
        MONGOSQL_AUTH_LOG("%s","      Acquiring credentials with username");
        principal_name = username;
    } else {
        // If both a username and a password were provided,
        // bundle username and password into auth_data.
        MONGOSQL_AUTH_LOG("%s","      Acquiring credentials with username and password");

        #ifdef _UNICODE
          auth_identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        #else
          auth_identity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
        #endif

        auth_identity.User = (LPSTR) username;
        auth_identity.UserLength = strlen(username);
        auth_identity.Password = (LPSTR) password;
        auth_identity.PasswordLength = strlen(password);
        auth_identity.Domain = NULL;
        auth_identity.DomainLength = 0;

        auth_data = &auth_identity;
    }

    client -> status = sspi_functions->AcquireCredentialsHandle(
        principal_name,       // principal name
        SSPI_PACKAGE_NAME,    // security package name
        SECPKG_CRED_OUTBOUND, // how credentials will be used
        NULL,                 // locally unique identifier for user
        auth_data,            // auth data
        NULL,                 // not used, should be NULL
        NULL,                 // not used, should be NULL
        &client->cred,        // OUTPUT: credential handle
        &timestamp            // OUTPUT: credential expiration time
    );

    if (client->status != SEC_E_OK) {
        return SSPI_ERROR;
    }

    return SSPI_OK;
}

int mongosql_auth_sspi_client_username(
    mongosql_auth_sspi_client *client,
    char** username
)
{
    SecPkgCredentials_Names names;
    client->status = sspi_functions->QueryCredentialsAttributes(&client->cred, SECPKG_CRED_ATTR_NAMES, &names);

    if (client->status != SEC_E_OK) {
        return SSPI_ERROR;
    }

    int len = strlen(names.sUserName) + 1;
    *username = malloc(len);
    memcpy(*username, names.sUserName, len);

    sspi_functions->FreeContextBuffer(names.sUserName);

    return SSPI_OK;
}

int mongosql_auth_sspi_client_negotiate(
    mongosql_auth_sspi_client *client,
    PVOID input,
    ULONG input_length,
    PVOID* output,
    ULONG* output_length
)
{
    SecBufferDesc inbuf;
    SecBuffer in_bufs[1];
    SecBufferDesc outbuf;
    SecBuffer out_bufs[1];

    if (client->has_ctx > 0) {
        inbuf.ulVersion = SECBUFFER_VERSION;
        inbuf.cBuffers = 1;
        inbuf.pBuffers = in_bufs;
        in_bufs[0].pvBuffer = input;
        in_bufs[0].cbBuffer = input_length;
        in_bufs[0].BufferType = SECBUFFER_TOKEN;
    }

    outbuf.ulVersion = SECBUFFER_VERSION;
    outbuf.cBuffers = 1;
    outbuf.pBuffers = out_bufs;
    out_bufs[0].pvBuffer = NULL;
    out_bufs[0].cbBuffer = 0;
    out_bufs[0].BufferType = SECBUFFER_TOKEN;

    ULONG context_attr = 0;

    MONGOSQL_AUTH_LOG("%s","      Initiating SSPI security context");
    client->status = sspi_functions->InitializeSecurityContext(
        &client->cred,                             // credentials handle
        client->has_ctx > 0 ? &client->ctx : NULL, // CtxtHandle
        (LPSTR) client->spn,                       // service principal name
        ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE, // request flags
        0,                                   // reserved, must be 0
        SECURITY_NETWORK_DREP,               // byte order
        client->has_ctx > 0 ? &inbuf : NULL, // input
        0,                                   // reserved, must be 0
        &client->ctx,                        // OUTPUT: CtxtHandle
        &outbuf,                             // OUTPUT: output data
        &context_attr,                       // OUTPUT: context attribute flags
        NULL                                 // OUTPUT: context expiration time
    );

    if (client->status != SEC_E_OK && client->status != SEC_I_CONTINUE_NEEDED) {
        return SSPI_ERROR;
    }

    client->has_ctx = 1;

    *output = malloc(out_bufs[0].cbBuffer);
    *output_length = out_bufs[0].cbBuffer;
    memcpy(*output, out_bufs[0].pvBuffer, *output_length);
    sspi_functions->FreeContextBuffer(out_bufs[0].pvBuffer);

    if (client->status == SEC_I_CONTINUE_NEEDED) {
        return SSPI_CONTINUE;
    }

    return SSPI_OK;
}

int mongosql_auth_sspi_client_wrap_msg(
    mongosql_auth_sspi_client *client,
    PVOID input,
    ULONG input_length,
    PVOID* output,
    ULONG* output_length
)
{
    SecPkgContext_Sizes sizes;

    client->status = sspi_functions->QueryContextAttributes(&client->ctx, SECPKG_ATTR_SIZES, &sizes);
    if (client->status != SEC_E_OK) {
        return SSPI_ERROR;
    }

    char *msg = malloc((sizes.cbSecurityTrailer + input_length + sizes.cbBlockSize) * sizeof(char));
    memcpy(&msg[sizes.cbSecurityTrailer], input, input_length);

    SecBuffer wrap_bufs[3];
    SecBufferDesc wrap_buf_desc;
    wrap_buf_desc.cBuffers = 3;
    wrap_buf_desc.pBuffers = wrap_bufs;
    wrap_buf_desc.ulVersion = SECBUFFER_VERSION;

    wrap_bufs[0].cbBuffer = sizes.cbSecurityTrailer;
    wrap_bufs[0].BufferType = SECBUFFER_TOKEN;
    wrap_bufs[0].pvBuffer = msg;

    wrap_bufs[1].cbBuffer = input_length;
    wrap_bufs[1].BufferType = SECBUFFER_DATA;
    wrap_bufs[1].pvBuffer = msg + sizes.cbSecurityTrailer;

    wrap_bufs[2].cbBuffer = sizes.cbBlockSize;
    wrap_bufs[2].BufferType = SECBUFFER_PADDING;
    wrap_bufs[2].pvBuffer = msg + sizes.cbSecurityTrailer + input_length;

    client->status = sspi_functions->EncryptMessage(&client->ctx, SECQOP_WRAP_NO_ENCRYPT, &wrap_buf_desc, 0);
    if (client->status != SEC_E_OK) {
        free(msg);
        return SSPI_ERROR;
    }

    *output_length = wrap_bufs[0].cbBuffer + wrap_bufs[1].cbBuffer + wrap_bufs[2].cbBuffer;
    *output = malloc(*output_length);

    memcpy(*output, wrap_bufs[0].pvBuffer, wrap_bufs[0].cbBuffer);
    memcpy((PVOID)((ULONG_PTR)*output + wrap_bufs[0].cbBuffer), wrap_bufs[1].pvBuffer, wrap_bufs[1].cbBuffer);
    memcpy((PVOID)((ULONG_PTR)*output + wrap_bufs[0].cbBuffer + wrap_bufs[1].cbBuffer), wrap_bufs[2].pvBuffer, wrap_bufs[2].cbBuffer);

    free(msg);

    return SSPI_OK;
}

int mongosql_auth_sspi_client_destroy(
    mongosql_auth_sspi_client *client
)
{
    if (client->has_ctx > 0) {
        sspi_functions->DeleteSecurityContext(&client->ctx);
    }

    sspi_functions->FreeCredentialsHandle(&client->cred);

    return SSPI_OK;
}
