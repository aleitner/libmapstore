#include "mapstore.h"

MAPSTORE_API int initialize_ctx(mapstore_ctx *ctx, char *config_path) {
    printf("Initializing mapstore context\n");
    int status = 0;

    char *config_raw = NULL;

    if (!config_path) {
        //TODO: Use default
       printf("Missing config path\n");
       status = 1;
       goto end_initalize;
    }

    int ret = read_config(config_path, &config_raw);

    if (ret == 1) {
       printf("Could not read config\n");
       status = 1;
       goto end_initalize;
    }

    json_object * jobj = json_tokener_parse(config_raw);
    printf("Config:\n%s\n",json_object_to_json_string(jobj));

end_initalize:
    if (config_raw) {
        free(config_raw);
    }

    return status;
}


MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API int retrieve_data(mapstore_ctx *ctx, uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API int delete_data(mapstore_ctx *ctx, uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API json_object *get_data_info(mapstore_ctx *ctx, uint8_t *hash) {
    printf("I'm here!");
}

MAPSTORE_API json_object *get_store_info(mapstore_ctx *ctx) {
    printf("I'm here!");;
}
