#include "mapstore.h"

MAPSTORE_API int start_server(char *config_path) {
    int status = 0;
    printf("I'm here!");

    struct MHD_Daemon *daemon = NULL;
    uv_loop_t *loop = uv_default_loop();
    sqlite3 *db = NULL;

    char *config_raw = NULL;

    if (!config_path) {
       printf("Missing first argument: <config-path>\n");
       status = 1;
       goto end_program;
    }

    int ret = read_config(config_path, &config_raw);

    if (ret == 1) {
       printf("Could not read config\n");
       status = 1;
       goto end_program;
    }

    json_object * jobj = json_tokener_parse(config_raw);
    printf("Config:\n%s\n",json_object_to_json_string(jobj));

    // TODO handle arguments

    char *db_path = "./shards.sqlite";

    if (sqlite3_open(db_path, &db)) {
       fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
       status = 1;
       goto end_program;
    }

    int port = 4001;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_DUAL_STACK,
                             port,
                             NULL,
                             NULL,
                             &mapstore_connection,
                             NULL,
                             MHD_OPTION_NOTIFY_COMPLETED,
                             &mapstore_connection_completed,
                             NULL,
                             MHD_OPTION_END);

    if (NULL == daemon) {
       status = 1;
       goto end_program;
    }

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, signal_handler, SIGINT);

    bool more;
    do {
       more = uv_run(loop, UV_RUN_ONCE);
       if (more == false) {
           more = uv_loop_alive(loop);
           if (uv_run(loop, UV_RUN_NOWAIT) != 0) {
               more = true;
           }
       }

    } while (more == true);


    end_program:

    if (loop) {
       uv_loop_close(loop);
    }

    if (daemon) {
       MHD_stop_daemon(daemon);
    }

    if (db) {
       sqlite3_close(db);
    }

    if (config_raw) {
       free(config_raw);
    }


    return 0;
}

MAPSTORE_API int stop_server() {
    printf("I'm here!");

    return 0;
}


MAPSTORE_API int store_data(int fd, uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API int retrieve_data(uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API int delete_data(uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API json_object *get_data_info(uint8_t *hash) {
    printf("I'm here!");
}

MAPSTORE_API json_object *get_store_info() {
    printf("I'm here!");;
}
