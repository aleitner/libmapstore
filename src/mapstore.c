#include "mapstore.h"

MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts) {
    printf("Initializing mapstore context\n");
    int status = 0;

    ctx->allocation_size = opts.allocation_size;
    ctx->map_size = opts.map_size;

end_initalize:

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
