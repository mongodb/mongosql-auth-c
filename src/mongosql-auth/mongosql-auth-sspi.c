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
        mongosql_auth_sspi_log_error(&sasl->client, "initializing client", errmsg);
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

    mongosql_auth_log("%s: %d","    Entering SSPI auth step. SASL state: ", sasl->state);
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
            mongosql_auth_log("%s","      Done initiating context");
            return SASL_OK;
        } else if (status == SSPI_CONTINUE) {
            mongosql_auth_log("%s","      Continue needed for SSPI auth");
            return SASL_OK;
        } else {
            mongosql_auth_sspi_log_error(&sasl->client, "negotiating with server", error);
            return SASL_ERR;
        }

    case SASL_CONTEXT_COMPLETE:
        // Get the client username from the credential.
        status = mongosql_auth_sspi_client_username(&sasl->client, &username);
        if (status != SSPI_OK) {
            mongosql_auth_sspi_log_error(&sasl->client, "getting client display name", error);
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
            mongosql_auth_sspi_log_error(&sasl->client, "wrapping quality of protection message", error);
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

    if (!username || strlen(username) == 0) {
        // If no username was provided, auth with the default credentials.
        // So we don't set principal_name or auth_data here.
        mongosql_auth_log("%s","      Acquiring default credentials");
    } else if (!password || strlen(password) == 0) {
        // If a username was provided but no password,
        // pass in the username as principal_name but no auth_data.
        mongosql_auth_log("%s","      Acquiring credentials with username");
        principal_name = username;
    } else {
        // If both a username and a password were provided,
        // bundle username and password into auth_data.
        mongosql_auth_log("%s","      Acquiring credentials with username and password");

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

    mongosql_auth_log("%s","      Initiating SSPI security context");
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

void mongosql_auth_sspi_log_error(
    mongosql_auth_sspi_client *client,
    const char *prefix,
    char **errmsg
) {
    char *tmp = NULL;

    mongosql_auth_sspi_error_desc(client->status, &tmp);
    mongosql_auth_log("      SSPI Error %s: %s (0x%x)", prefix, tmp, client->status);

    if (errmsg) {
        *errmsg = tmp;
    } else {
        free(tmp);
    }
}

void mongosql_auth_sspi_error_desc(
    int status,
    char **desc
) {
    char *s;
    char *msg;

    switch (status) {
    case SEC_E_ALGORITHM_MISMATCH:
      s = "The client and server cannot communicate because they do not possess a common algorithm.";
      break;
    case SEC_E_BAD_BINDINGS:
      s = "The SSPI channel bindings supplied by the client are incorrect.";
      break;
    case SEC_E_BAD_PKGID:
      s = "The requested package identifier does not exist.";
      break;
    case SEC_E_BUFFER_TOO_SMALL:
      s = "The buffers supplied to the function are not large enough to contain the information.";
      break;
    case SEC_E_CANNOT_INSTALL:
      s = "The security package cannot initialize successfully and should not be installed.";
      break;
    case SEC_E_CANNOT_PACK:
      s = "The package is unable to pack the context.";
      break;
    case SEC_E_CERT_EXPIRED:
      s = "The received certificate has expired.";
      break;
    case SEC_E_CERT_UNKNOWN:
      s = "An unknown error occurred while processing the certificate.";
      break;
    case SEC_E_CERT_WRONG_USAGE:
      s = "The certificate is not valid for the requested usage.";
      break;
    case SEC_E_CONTEXT_EXPIRED:
      s = "The application is referencing a context that has already been closed. A properly written application should not receive this error.";
      break;
    case SEC_E_CROSSREALM_DELEGATION_FAILURE:
      s = "The server attempted to make a Kerberos-constrained delegation request for a target outside the server's realm.";
      break;
    case SEC_E_CRYPTO_SYSTEM_INVALID:
      s = "The cryptographic system or checksum function is not valid because a required function is unavailable.";
      break;
    case SEC_E_DECRYPT_FAILURE:
      s = "The specified data could not be decrypted.";
      break;
    case SEC_E_DELEGATION_REQUIRED:
      s = "The requested operation cannot be completed. The computer must be trusted for delegation";
      break;
    case SEC_E_DOWNGRADE_DETECTED:
      s = "The system detected a possible attempt to compromise security. Verify that the server that authenticated you can be contacted.";
      break;
    case SEC_E_ENCRYPT_FAILURE:
      s = "The specified data could not be encrypted.";
      break;
    case SEC_E_ILLEGAL_MESSAGE:
      s = "The message received was unexpected or badly formatted.";
      break;
    case SEC_E_INCOMPLETE_CREDENTIALS:
      s = "The credentials supplied were not complete and could not be verified. The context could not be initialized.";
      break;
    case SEC_E_INCOMPLETE_MESSAGE:
      s = "The message supplied was incomplete. The signature was not verified.";
      break;
    case SEC_E_INSUFFICIENT_MEMORY:
      s = "Not enough memory is available to complete the request.";
      break;
    case SEC_E_INTERNAL_ERROR:
      s = "An error occurred that did not map to an SSPI error code.";
      break;
    case SEC_E_INVALID_HANDLE:
      s = "The handle passed to the function is not valid.";
      break;
    case SEC_E_INVALID_TOKEN:
      s = "The token passed to the function is not valid.";
      break;
    case SEC_E_ISSUING_CA_UNTRUSTED:
      s = "An untrusted certification authority (CA) was detected while processing the smart card certificate used for authentication.";
      break;
    case SEC_E_ISSUING_CA_UNTRUSTED_KDC:
      s = "An untrusted CA was detected while processing the domain controller certificate used for authentication. The system event log contains additional information.";
      break;
    case SEC_E_KDC_CERT_EXPIRED:
      s = "The domain controller certificate used for smart card logon has expired.";
      break;
    case SEC_E_KDC_CERT_REVOKED:
      s = "The domain controller certificate used for smart card logon has been revoked.";
      break;
    case SEC_E_KDC_INVALID_REQUEST:
      s = "A request that is not valid was sent to the KDC.";
      break;
    case SEC_E_KDC_UNABLE_TO_REFER:
      s = "The KDC was unable to generate a referral for the service requested.";
      break;
    case SEC_E_KDC_UNKNOWN_ETYPE:
      s = "The requested encryption type is not supported by the KDC.";
      break;
    case SEC_E_LOGON_DENIED:
      s = "The logon has been denied";
      break;
    case SEC_E_MAX_REFERRALS_EXCEEDED:
      s = "The number of maximum ticket referrals has been exceeded.";
      break;
    case SEC_E_MESSAGE_ALTERED:
      s = "The message supplied for verification has been altered.";
      break;
    case SEC_E_MULTIPLE_ACCOUNTS:
      s = "The received certificate was mapped to multiple accounts.";
      break;
    case SEC_E_MUST_BE_KDC:
      s = "The local computer must be a Kerberos domain controller (KDC)";
      break;
    case SEC_E_NO_AUTHENTICATING_AUTHORITY:
      s = "No authority could be contacted for authentication.";
      break;
    case SEC_E_NO_CREDENTIALS:
      s = "No credentials are available.";
      break;
    case SEC_E_NO_IMPERSONATION:
      s = "No impersonation is allowed for this context.";
      break;
    case SEC_E_NO_IP_ADDRESSES:
      s = "Unable to accomplish the requested task because the local computer does not have any IP addresses.";
      break;
    case SEC_E_NO_KERB_KEY:
      s = "No Kerberos key was found.";
      break;
    case SEC_E_NO_PA_DATA:
      s = "Policy administrator (PA) data is needed to determine the encryption type";
      break;
    case SEC_E_NO_S4U_PROT_SUPPORT:
      s = "The Kerberos subsystem encountered an error. A service for user protocol request was made against a domain controller which does not support service for a user.";
      break;
    case SEC_E_NO_TGT_REPLY:
      s = "The client is trying to negotiate a context and the server requires a user-to-user connection";
      break;
    case SEC_E_NOT_OWNER:
      s = "The caller of the function does not own the credentials.";
      break;
    case SEC_E_OK:
      s = "The operation completed successfully.";
      break;
    case SEC_E_OUT_OF_SEQUENCE:
      s = "The message supplied for verification is out of sequence.";
      break;
    case SEC_E_PKINIT_CLIENT_FAILURE:
      s = "The smart card certificate used for authentication is not trusted.";
      break;
    case SEC_E_PKINIT_NAME_MISMATCH:
      s = "The client certificate does not contain a valid UPN or does not match the client name in the logon request.";
      break;
    case SEC_E_QOP_NOT_SUPPORTED:
      s = "The quality of protection attribute is not supported by this package.";
      break;
    case SEC_E_REVOCATION_OFFLINE_C:
      s = "The revocation status of the smart card certificate used for authentication could not be determined.";
      break;
    case SEC_E_REVOCATION_OFFLINE_KDC:
      s = "The revocation status of the domain controller certificate used for smart card authentication could not be determined. The system event log contains additional information.";
      break;
    case SEC_E_SECPKG_NOT_FOUND:
      s = "The security package was not recognized.";
      break;
    case SEC_E_SECURITY_QOS_FAILED:
      s = "The security context could not be established due to a failure in the requested quality of service (for example";
      break;
    case SEC_E_SHUTDOWN_IN_PROGRESS:
      s = "A system shutdown is in progress.";
      break;
    case SEC_E_SMARTCARD_CERT_EXPIRED:
      s = "The smart card certificate used for authentication has expired.";
      break;
    case SEC_E_SMARTCARD_CERT_REVOKED:
      s = "The smart card certificate used for authentication has been revoked. Additional information may exist in the event log.";
      break;
    case SEC_E_SMARTCARD_LOGON_REQUIRED:
      s = "Smart card logon is required and was not used.";
      break;
    case SEC_E_STRONG_CRYPTO_NOT_SUPPORTED:
      s = "The other end of the security negotiation requires strong cryptography";
      break;
    case SEC_E_TARGET_UNKNOWN:
      s = "The target was not recognized.";
      break;
    case SEC_E_TIME_SKEW:
      s = "The clocks on the client and server computers do not match.";
      break;
    case SEC_E_TOO_MANY_PRINCIPALS:
      s = "The KDC reply contained more than one principal name.";
      break;
    case SEC_E_UNFINISHED_CONTEXT_DELETED:
      s = "A security context was deleted before the context was completed. This is considered a logon failure.";
      break;
    case SEC_E_UNKNOWN_CREDENTIALS:
      s = "The credentials provided were not recognized.";
      break;
    case SEC_E_UNSUPPORTED_FUNCTION:
      s = "The requested function is not supported.";
      break;
    case SEC_E_UNSUPPORTED_PREAUTH:
      s = "An unsupported preauthentication mechanism was presented to the Kerberos package.";
      break;
    case SEC_E_UNTRUSTED_ROOT:
      s = "The certificate chain was issued by an authority that is not trusted.";
      break;
    case SEC_E_WRONG_CREDENTIAL_HANDLE:
      s = "The supplied credential handle does not match the credential associated with the security context.";
      break;
    case SEC_E_WRONG_PRINCIPAL:
      s = "The target principal name is incorrect.";
      break;
    case SEC_I_COMPLETE_AND_CONTINUE:
      s = "The function completed successfully";
      break;
    case SEC_I_COMPLETE_NEEDED:
      s = "The function completed successfully";
      break;
    case SEC_I_CONTEXT_EXPIRED:
      s = "The message sender has finished using the connection and has initiated a shutdown. For information about initiating or recognizing a shutdown";
      break;
    case SEC_I_CONTINUE_NEEDED:
      s = "The function completed successfully";
      break;
    case SEC_I_INCOMPLETE_CREDENTIALS:
      s = "The credentials supplied were not complete and could not be verified. Additional information can be returned from the context.";
      break;
    case SEC_I_LOCAL_LOGON:
      s = "The logon was completed";
      break;
    case SEC_I_NO_LSA_CONTEXT:
      s = "There is no LSA mode context associated with this context.";
      break;
    case SEC_I_RENEGOTIATE:
      s = "The context data must be renegotiated with the peer.";
      break;
    default:
      s = "Unknown SSPI error.";
    }

    msg = malloc(strlen(s) + 1);
    memcpy(msg, s, strlen(s) + 1);
    *desc = msg;
}
