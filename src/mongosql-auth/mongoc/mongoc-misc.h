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

#ifndef MONGOC_MISC_H
#define MONGOC_MISC_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <my_global.h>

#define MONGOC_ERROR_SCRAM 1
#define MONGOC_ERROR_SCRAM_NOT_DONE 2
#define MONGOC_ERROR_SCRAM_PROTOCOL_ERROR 3

typedef struct {
   uint32_t domain;
   uint32_t code;
   char message[504];
} bson_error_t;

void
bson_set_error (bson_error_t *error, /* OUT */
                uint32_t domain,     /* IN */
                uint32_t code,       /* IN */
                const char *format,  /* IN */
                ...);                 /* IN */

char *
bson_strdup_printf (const char *format, /* IN */
                    ...);                /* IN */

int64_t
bson_ascii_strtoll (const char *s, char **e, int base);

char *
_mongoc_hex_md5 (const char *input);

#endif /* MONGOC_MISC_H */
