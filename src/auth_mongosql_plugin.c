/*
 * Copyright 2017 MongoDB, Inc.
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

#include <my_global.h>
#include <mysql/plugin_auth.h>
#include <mysql/client_plugin.h>
#include <mysql/service_my_plugin_log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <mysql.h>
#include "bson.h"
#include "mongoc-error.h"
#include "mongoc-scram.c"

#define MAX_MECHANISM_LENGTH 1024

/**
  Authenticate the client using the MongoDB MySQL Authentication Plugin Protocol.

  @param vio Provides plugin access to communication channel
  @param mysql Client connection handler

  @return Error status
    @retval CR_ERROR An error occurred.
    @retval CR_OK Authentication succeeded.
*/
static int mongosql_auth(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql)
{

    /* read auth-data */
    unsigned char *pkt;
    int pkt_len;
    pkt_len = vio->read_packet(vio, &pkt);
    if (pkt_len < 0) {
        fprintf(stderr, "ERROR: failed while reading auth-data from initial handshake\n");
        return CR_ERROR;
    }

    /* parse the contents of auth-data */
    uint8_t major_version;
    uint8_t minor_version;
    memcpy(&major_version, pkt, 1);
    memcpy(&minor_version, pkt+1, 1);
    fprintf(stderr, "received auth-data from server (%d bytes)\n", pkt_len);
    fprintf(stderr, "    major_version: %d\n", major_version);
    fprintf(stderr, "    minor_version: %d\n", minor_version);

    /* write 0 bytes */
    if (vio->write_packet(vio, (const unsigned char *) "", 1)) {
        fprintf(stderr, "ERROR: failed while writing zero-byte response\n");
        return CR_ERROR;
    }
    fprintf(stderr, "sent empty handshake response\n");

    /* first auth-more-data */
    pkt_len = vio->read_packet(vio, &pkt);
    if (pkt_len < 0) {
        fprintf(stderr, "ERROR: failed while reading first auth-more-data\n");
        return CR_ERROR;
    }

    unsigned char *mechanism;
    uint32_t num_conversations;
    mechanism = pkt;
    memcpy(&num_conversations, pkt+strlen((const char *)mechanism)+1, 4);
    fprintf(stderr, "received first auth-more-data (%d bytes)\n", pkt_len);
    fprintf(stderr, "    mechanism: '%s'\n", mechanism);
    fprintf(stderr, "    num_conversations: %d\n", num_conversations);

    if(strcmp(mechanism, "SCRAM-SHA-1") == 0) {
        return auth_scram(vio, mysql, num_conversations);
    } else if (strcmp(mechanism, "PLAIN") == 0) {
        return auth_plain(vio, mysql, num_conversations);
    } else {
        fprintf(stderr, "ERROR: plugin does not support mechanism '%s'\n", mechanism);
        return CR_ERROR;
    }
}

int auth_scram(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql, uint32_t num_conversations) {

    /* remove parameters from username */
    char *ptr = strchr(mysql->user, '?');
    if(ptr != NULL) {
        *ptr = '\0';
    }

    /* initialize scram conversations */
    mongoc_scram_t conversations[num_conversations];
    for (unsigned int i=0; i<num_conversations; i++) {
        mongoc_scram_t scram;
        _mongoc_scram_init (&scram);
        _mongoc_scram_set_user (&scram, mysql->user);
        _mongoc_scram_set_pass (&scram, mysql->passwd);
        conversations[i] = scram;
    }

    fprintf(stderr, "initialized scram conversations\n");
    for (unsigned int i=0; i<num_conversations; i++) {
        fprintf(stderr, "    conversation: %d\n", i);
        fprintf(stderr, "        user: %s\n", conversations[i].user);
        fprintf(stderr, "        pass: %s\n", conversations[i].pass);
    }

    /* initialize scram buffers */
    uint8_t done[num_conversations];
    uint8_t *bufs[num_conversations];
    uint32_t buf_lens[num_conversations];
    for (unsigned int i=0; i<num_conversations; i++) {
        bufs[i] = malloc(4096);
        buf_lens[i] = 0;
    }

    fprintf(stderr, "initialized scram conversation buffers\n");

    while (1) {

        /*
         * for each scram conversation, process scram_inbuf
         * and populate scram_outbuf
         */
        for(unsigned int i=0; i<num_conversations; i++) {

            if (conversations[i].step == 2) {
                done[i] = 1;
            }

            bson_error_t error;
            my_bool success;
            success = _mongoc_scram_step (
                &conversations[i],
                bufs[i],
                buf_lens[i],
                bufs[i],
                (uint32_t)(4096 * sizeof(uint8_t)),
                &buf_lens[i],
                &error
            );

            if (!success) {
                fprintf(stderr, "ERROR: failed while executing scram step %d for conversation %d\n", conversations[i].step, i);
                fprintf(stderr, "    message: '%s'\n", error.message);
                return CR_ERROR;
            }
        }

        fprintf(stderr, "executed scram step %d for all conversations\n", conversations[0].step);

        /* allocate buffer for auth protocol payload */
        uint32_t conversation_payload_len = buf_lens[0]; // TODO: verify all buf_lens same
        uint32_t conversation_len = conversation_payload_len + 5;
        uint32_t mongosql_auth_data_len = conversation_len * num_conversations;
        unsigned char *mongosql_auth_data = malloc(mongosql_auth_data_len);

        /*
         * for each scram conversation, process scram_outbuf
         * and populate the appropriate piece of mongosql_auth_data
         */
        unsigned char *conversation_data = mongosql_auth_data;
        for(unsigned int i=0; i<num_conversations; i++) {
            memcpy(conversation_data, &done[i], 1);
            memcpy(conversation_data+1, &buf_lens[i], 4);
            memcpy(conversation_data+5, bufs[i], buf_lens[i]);

            conversation_data += conversation_len;
        }

        fprintf(stderr, "built mongosql_auth_data buffer\n");

        /* write mongosql_auth_data to the wire */
        if (vio->write_packet(vio, mongosql_auth_data, mongosql_auth_data_len)) {
            fprintf(stderr, "ERROR: failed while writing scram step %d\n", conversations[0].step);
            return CR_ERROR;
        }

        fprintf(stderr, "sent scram step %d\n", conversations[0].step);
        for(unsigned int i=0; i<num_conversations; i++) {
            fprintf(stderr, "    conversation: %d\n", i);
            fprintf(stderr, "        complete: %d\n", done[i]);
            fprintf(stderr, "        payload_len: %d\n", buf_lens[i]);
            fprintf(stderr, "        payload: '%s'\n", bufs[i]);
        }

        if (done[0] == 1) {
            goto finished;
        }

        /* read server reply */
        unsigned char *pkt;
        int pkt_len;
        pkt_len = vio->read_packet(vio, &pkt);
        if (pkt_len < 0) {
            fprintf(stderr, "ERROR: failed while reading server reply to scram step %d\n", conversations[0].step);
            return CR_ERROR;
        }
        fprintf(stderr, "received scram step %d response\n", conversations[0].step);
        fprintf(stderr, "    length: %d\n", pkt_len);

        /* read the server reply and populate scram_inbuf s */
        for(unsigned int i=0; i<num_conversations; i++) {
            fprintf(stderr, "    conversation: %d\n", i);
            uint32_t payload_len;
            memcpy(&payload_len, pkt, 4);
            pkt += 4;
            buf_lens[i] = payload_len;
            fprintf(stderr, "        payload_len: %d\n", payload_len);

            memcpy(bufs[i], pkt, payload_len);
            pkt += payload_len;
            fprintf(stderr, "        payload: '%s'\n", bufs[i]);
        }
    }

    fprintf(stderr, "%s", "SCRAM: authenticated");

finished:
    for(unsigned int i=0; i<num_conversations; i++) {
        _mongoc_scram_destroy (&conversations[i]);
    }

    return CR_OK;
}

int auth_plain(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql, uint32_t num_conversations) {

    /* remove parameters from username */
    char *ptr = strchr(mysql->user, '?');
    if(ptr != NULL) {
        *ptr = '\0';
    }

    /* base64-encode username and password */
    char buf[4096];
    int buflen = 0;
    size_t len;
    unsigned char *str;
    str = bson_strdup_printf ("%c%s%c%s", '\0', mysql->user, '\0', mysql->passwd);
    len = strlen (mysql->user) + strlen (mysql->passwd) + 2;
    buflen = mongoc_b64_ntop ((const uint8_t *) str, len, buf, sizeof buf);
    free (str);

    fprintf(stderr, "base64-encoded username and password\n");
    fprintf(stderr, "    username: %s\n", mysql->user);
    fprintf(stderr, "    password: %s\n", mysql->passwd);
    fprintf(stderr, "    base64: %s\n", buf);

    if (buflen == -1) {
        fprintf(stderr, "ERROR: failed base64 encoding message\n");
        return CR_ERROR;
    }

    /* allocate buffer for auth protocol payload */
    uint32_t conversation_payload_len = buflen;
    uint32_t conversation_len = conversation_payload_len + 5;
    uint32_t mongosql_auth_data_len = conversation_len * num_conversations;
    unsigned char *mongosql_auth_data = malloc(mongosql_auth_data_len);

    /* populate buffer with PLAIN payload for each conversation */
    unsigned char *conversation_data = mongosql_auth_data;
    for(unsigned int i=0; i<num_conversations; i++) {
        uint8_t done = 1;
        memcpy(conversation_data, &done, 1);
        memcpy(conversation_data+1, &buflen, 4);
        memcpy(conversation_data+5, buf, buflen);

        conversation_data += conversation_len;
    }

    fprintf(stderr, "built mongosql_auth_data buffer\n");

    /* write mongosql_auth_data to the wire */
    if (vio->write_packet(vio, mongosql_auth_data, mongosql_auth_data_len)) {
        fprintf(stderr, "ERROR: failed while writing PLAIN payload\n");
        return CR_ERROR;
    }

    return CR_OK;
}

mysql_declare_client_plugin(AUTHENTICATION)
    "mongosql_auth",
    "MongoDB",
    "MongoDB MySQL Authentication Plugin",
    {0,1,0},
    "GPL",
    NULL,
    NULL,
    NULL,
    NULL,
    mongosql_auth
mysql_end_client_plugin;
