#include "mapstore.h"

MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts) {
    printf("Initializing mapstore context\n");
    int status = 0;

    if (!opts.allocation_size) {
        printf("Can't initialize mapstore context. \
                Missing required allocation size\n");
        status = 1;
        goto end_initalize;
    }

    ctx->allocation_size = opts.allocation_size;
    ctx->map_size = (opts.map_size) ? opts.map_size : 2147483648;

    char *base_path = NULL;
    if (opts.path != NULL) {
        if (opts.path[strlen(opts.path)] == '/') {
            opts.path[strlen(opts.path)] = '\0';
        }
        base_path = strdup(opts.path);
    } else {
        base_path = calloc(2, sizeof(char));
        strcpy(base_path, ".");
    }

    printf("base_path: %s\n", base_path);

    ctx->mapstore_path = calloc(strlen(base_path) + 9, sizeof(char));
    sprintf(ctx->mapstore_path, "%s/shards/", base_path);

    ctx->database_path = calloc(strlen(base_path) + 15, sizeof(char));
    sprintf(ctx->database_path, "%s/shards.sqlite", base_path);


end_initalize:
    if (base_path) {
        free(base_path);
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
