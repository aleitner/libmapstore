#include <microhttpd.h>
#include <stdio.h>
#include <sqlite3.h>
#include <json-c/json.h>
#include <assert.h>
#include <uv.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>

int read_config(char *config_path, char **config_raw);

void signal_handler(uv_signal_t *req, int signum);

void mapstore_connection_completed(void *cls,
                                            struct MHD_Connection *connection,
                                            void **con_cls,
                                            enum MHD_RequestTerminationCode toe);

int mapstore_connection(void *cls,
                                 struct MHD_Connection *connection,
                                 const char *url,
                                 const char *method,
                                 const char *version,
                                 const char *upload_data,
                                 size_t *upload_data_size,
                                 void **con_cls);
