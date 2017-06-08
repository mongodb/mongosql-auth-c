/*
 * Copyright 2017 MongoDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mongosql-auth.h"

#define MONGOSQL_AUTH_PROTOCOL_MAJOR_VERSION 1
#define MONGOSQL_AUTH_PROTOCOL_MINOR_VERSION 0

/* initialize the plugin state with the provided fields */
void
_mongosql_auth_init(mongosql_auth_t *plugin, MYSQL_PLUGIN_VIO *vio) {

    MONGOSQL_AUTH_LOG("%s", "Initializing auth plugin");

    /* initialize fields with provided parameters */
    plugin->vio = vio;

    /* initialize other fields */
    plugin->status = CR_OK;
    plugin->error_msg = NULL;
    plugin->conversations = NULL;
    plugin->num_conversations = 0;
}

void
_mongosql_auth_destroy(mongosql_auth_t *plugin) {

    /* free strduped strings */
    free(plugin->error_msg);

    /* call the conversation destructors */
    for (unsigned int i=0; i<plugin->num_conversations; i++) {
        _mongosql_auth_conversation_destroy(&plugin->conversations[i]);
    }

    /* free the conversations array */
    free(plugin->conversations);
}

/* execute the first steps of the conversation and update the plugin with auth data */
void
_mongosql_auth_start(mongosql_auth_t *plugin, const char *username, const char *password) {
    unsigned char *pkt;
    int pkt_len;
    uint8_t major_version;
    uint8_t minor_version;
    unsigned char *mechanism;

    /* read auth-data */
    MONGOSQL_AUTH_LOG("%s", "Reading auth-data from server");
    pkt_len = plugin->vio->read_packet(plugin->vio, &pkt);
    if (pkt_len < 0) {
        _mongosql_auth_set_error(plugin, "failed reading auth-data from initial handshake");
        return;
    }

    /* parse the contents of auth-data */
    memcpy(&major_version, pkt, 1);
    memcpy(&minor_version, pkt+1, 1);
    MONGOSQL_AUTH_LOG("Server protocol version: %d.%d", major_version, minor_version);
    MONGOSQL_AUTH_LOG("Client protocol version: %d.%d", MONGOSQL_AUTH_PROTOCOL_MAJOR_VERSION,
                                                        MONGOSQL_AUTH_PROTOCOL_MINOR_VERSION);

    /* validate protocol version */
    if (major_version != MONGOSQL_AUTH_PROTOCOL_MAJOR_VERSION ||
        minor_version != MONGOSQL_AUTH_PROTOCOL_MINOR_VERSION) {
        _mongosql_auth_set_error(plugin, "server protocol version incompatible with client protocol version");
        return;
    }

    /* write 0 bytes */
    MONGOSQL_AUTH_LOG("%s", "Writing empty response to server");
    if (plugin->vio->write_packet(plugin->vio, (const unsigned char *) "", 1)) {
        _mongosql_auth_set_error(plugin, "failed while reading zero-byte response to server");
        return;
    }

    /* read first auth-more-data */
    MONGOSQL_AUTH_LOG("%s", "Reading first auth-more-data from server");
    pkt_len = plugin->vio->read_packet(plugin->vio, &pkt);
    if (pkt_len < 0) {
        _mongosql_auth_set_error(plugin, "failed while reading first auth-more-data");
        return;
    }

    /* set the plugin's num_conversations field */
    mechanism = pkt;
    memcpy(&plugin->num_conversations, pkt+strlen(mechanism)+1, 4);
    MONGOSQL_AUTH_LOG("    mechanism: %s", mechanism);
    MONGOSQL_AUTH_LOG("    num_conversations: %u", plugin->num_conversations);

    /* allocate and initialize conversations */
    MONGOSQL_AUTH_LOG("Initializing %d conversation structs", plugin->num_conversations);
    plugin->conversations = calloc(plugin->num_conversations, sizeof(mongosql_auth_conversation_t));
    for (unsigned int i=0; i<plugin->num_conversations; i++) {
        _mongosql_auth_conversation_init(&plugin->conversations[i], username, password, mechanism);
    }
}

/* read server challenge, process it, send response */
void
_mongosql_auth_step(mongosql_auth_t *plugin) {
    mongosql_auth_conversation_t *conv;

    /* if there is an error, stop */
    if (_mongosql_auth_has_error(plugin)) {
        return;
    }

    MONGOSQL_AUTH_LOG("%s", "Stepping mongosql_auth protocol");

    /* step each individual conversation */
    for (unsigned int i=0; i<plugin->num_conversations; i++) {
        conv = &plugin->conversations[i];
        _mongosql_auth_conversation_step(conv);
    }
}

/* read data from the wire and split it up into individual conversations */
void
_mongosql_auth_read_payload(mongosql_auth_t *plugin) {
    unsigned char *pkt;
    int pkt_len;
    mongosql_auth_conversation_t *conv;

    /* if we are done, we don't read another server payload */
    if (_mongosql_auth_is_done(plugin)) {
        return;
    }

    MONGOSQL_AUTH_LOG("%s", "Reading payload from server");

    /* read server reply */
    pkt_len = plugin->vio->read_packet(plugin->vio, &pkt);
    if (pkt_len < 0) {
        _mongosql_auth_set_error(plugin, "failed reading payload from server");
        return;
    }

    /* take the server reply and populate each conversation's buffer */
    for(unsigned int i=0; i<plugin->num_conversations; i++) {
        conv = &plugin->conversations[i];
        memcpy(&conv->buf_len, pkt, 4);
        pkt += 4;
        memcpy(conv->buf, pkt, conv->buf_len);
        pkt += conv->buf_len;
    }
}

/* bundle up data from individual conversations and send it on the wire */
void
_mongosql_auth_write_payload(mongosql_auth_t *plugin) {
    int err;
    mongosql_auth_conversation_t *conv;
    uint32_t conversation_payload_len;
    uint32_t conversation_len;
    uint32_t mongosql_auth_data_len;
    unsigned char *mongosql_auth_data;
    unsigned char *conversation_data;

    /* if there is an error, stop */
    if (_mongosql_auth_has_error(plugin)) {
        MONGOSQL_AUTH_LOG("%s", "Not writing payload: error already encountered");
        return;
    }

    MONGOSQL_AUTH_LOG("%s", "Writing payload to server");

    /* allocate buffer for auth protocol payload */
    conversation_payload_len = plugin->conversations[0].buf_len;
    conversation_len = conversation_payload_len + 5;
    mongosql_auth_data_len = conversation_len * plugin->num_conversations;
    mongosql_auth_data = malloc(mongosql_auth_data_len);

    /*
     * for each conversation, take the client message and
     * populate the corresponding piece of mongosql_auth_data
     */
    conversation_data = mongosql_auth_data;
    for(unsigned int i=0; i<plugin->num_conversations; i++) {
        conv = &plugin->conversations[i];

        memcpy(conversation_data, &conv->done, 1);
        memcpy(conversation_data+1, &conv->buf_len, 4);
        memcpy(conversation_data+5, conv->buf, conv->buf_len);

        conversation_data += conversation_len;
    }

    /* write mongosql_auth_data to the wire */
    err = plugin->vio->write_packet(
            plugin->vio,
            mongosql_auth_data,
            mongosql_auth_data_len);
    if (err) {
        _mongosql_auth_set_error(plugin, "failed writing client response");
    }

    free (mongosql_auth_data);
}

void
_mongosql_auth_set_error(mongosql_auth_t *plugin, const char *msg) {
    if (_mongosql_auth_has_error(plugin)) {
        return;
    }
    plugin->error_msg = strdup(msg);
    plugin->status = CR_ERROR;
}

my_bool
_mongosql_auth_has_error(mongosql_auth_t *plugin) {
    mongosql_auth_conversation_t *conv;

    /* if we already have an error, return true */
    if (plugin->status == CR_ERROR) {
        return TRUE;
    }

    /*
     * check if any of the conversations have an error.
     * if so, make that error our error and return true.
     */
    for (unsigned int i=0; i<plugin->num_conversations; i++) {
        conv = &plugin->conversations[i];
        if (_mongosql_auth_conversation_has_error(conv)) {
            plugin->error_msg = strdup(conv->error_msg);
            plugin->status = CR_ERROR;
            return TRUE;
        }
    }

    return FALSE;
}

my_bool
_mongosql_auth_is_done(mongosql_auth_t *plugin) {
    if (_mongosql_auth_has_error(plugin)) {
        return TRUE;
    }

    for (unsigned int i=0; i<plugin->num_conversations; i++) {
        if (!_mongosql_auth_conversation_is_done(&plugin->conversations[i])) {
            return FALSE;
        }
    }

    return TRUE;
}
