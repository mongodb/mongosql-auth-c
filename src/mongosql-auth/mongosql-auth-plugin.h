
#include <mysql.h>
#include "mongoc/mongoc-misc.h"

int auth_scram(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql, uint32_t num_conversations);
int auth_plain(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql, uint32_t num_conversations);
