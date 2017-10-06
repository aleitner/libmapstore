#include "server.h"

int read_config(char *config_path, char **config_raw)
{
    FILE *config_file = fopen(config_path, "r");

    if (!config_file) {
        printf("Could not open config: %s\n", config_path);
        return 1;
    }

    // TODO read config
    struct stat st;
    stat(config_path, &st);
    int config_size = st.st_size;

    // Read config into config_raw
    *config_raw = calloc(config_size, sizeof(char));;
    fread(*config_raw, config_size, 1, config_file);

    return 0;
}

int mapstore_connection(void *cls,
                                 struct MHD_Connection *connection,
                                 const char *url,
                                 const char *method,
                                 const char *version,
                                 const char *upload_data,
                                 size_t *upload_data_size,
                                 void **con_cls)
{
    const char *encoding = MHD_lookup_connection_value(connection,
                                                       MHD_HEADER_KIND,
                                                       MHD_HTTP_HEADER_CONTENT_TYPE);

    if (NULL == *con_cls) {

        *con_cls = (void *)connection;

        return MHD_YES;
    }

    if (0 == strcmp(url, "/") && 0 == strcmp(method, "POST")) {
        // JSON-RPC API

        // TODO parse json

        // TODO match command

        // TODO dispatch to matching command

    } else if (0 == strncmp(url, "/shards/", 8)) {

        // SHARDS HTTP API

        // TODO get shard hash and token

        if (0 == strcmp(method, "POST")) {

            // TODO check token

            // TODO save shard data

        } else if (0 == strcmp(method, "GET")) {

            // TODO check token
            // TODO check shard exists
            // TODO send shard data

            int status_code = MHD_HTTP_OK;
            char *page = "Shard data!";
            int page_size = 11;
            char *sent_page = calloc(page_size + 1, sizeof(char));
            memcpy(sent_page, page, page_size);
            struct MHD_Response *response;

            response = MHD_create_response_from_buffer(page_size,
                                                       (void *) sent_page,
                                                       MHD_RESPMEM_MUST_FREE);

            int ret = MHD_queue_response(connection, status_code, response);
            if (ret == MHD_NO) {
                fprintf(stderr, "Response error\n");
            }

            MHD_destroy_response(response);

            return ret;

        }

    }

    return MHD_NO;

}

void mapstore_connection_completed(void *cls,
                                            struct MHD_Connection *connection,
                                            void **con_cls,
                                            enum MHD_RequestTerminationCode toe)
{
    *con_cls = NULL;
}

void signal_handler(uv_signal_t *req, int signum)
{
    printf("Shutting down!\n");
    uv_signal_stop(req);
}
