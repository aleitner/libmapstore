#include "mapstore.h"

/**
* Initialize everything
*/
MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts) {
    fprintf(stdout, "Initializing mapstore context\n");
    int status = 0;

    /* Initialized path variables */
    ctx->mapstore_path = NULL;
    ctx->database_path = NULL;

    if (!opts.allocation_size) {
        fprintf(stderr, "Can't initialize mapstore context. \
                Missing required allocation size\n");
        status = 1;
        goto end_initalize;
    }

    ctx->allocation_size = opts.allocation_size;
    ctx->map_size = (opts.map_size) ? opts.map_size : 2147483648;

    char *base_path = NULL;
    if (opts.path != NULL) {
        base_path = strdup(opts.path);
        if (base_path[strlen(base_path)-1] == separator()) {
            base_path[strlen(base_path)-1] = '\0';
        }
    } else {
        base_path = calloc(2, sizeof(char));
        strcpy(base_path, ".");
    }

    ctx->mapstore_path = calloc(strlen(base_path) + 9, sizeof(char));
    sprintf(ctx->mapstore_path, "%s%cshards%c", base_path, separator(), separator());

    if (create_directory(ctx->mapstore_path) != 0) {
        fprintf(stderr, "Could not create folder: %s\n", ctx->mapstore_path);
        status = 1;
        goto end_initalize;
    };

    ctx->database_path = calloc(strlen(base_path) + 15, sizeof(char));
    sprintf(ctx->database_path, "%s%cshards.sqlite", base_path, separator());

end_initalize:
    if (base_path) {
        free(base_path);
    }

    if (status == 1) {
        if (ctx->mapstore_path) {
            free(ctx->mapstore_path);
        }

        if (ctx->database_path) {
            free(ctx->database_path);
        }
    }

    return status;
}

/**
* Store data
*/
MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, uint8_t *hash) {
    fprintf(stdout, "Begin Store Data\n");
    int status = 0;

    sqlite3 *db = NULL;
    if (sqlite3_open(ctx->database_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_store_data;
    }

end_store_data:
    if (db) {
        sqlite3_close(db);
    }

    return status;
}

/**
* Retrieve data
*/
MAPSTORE_API int retrieve_data(mapstore_ctx *ctx, uint8_t *hash) {
    fprintf(stdout, "I'm here!");

    return 0;
}

MAPSTORE_API int delete_data(mapstore_ctx *ctx, uint8_t *hash) {
    fprintf(stdout, "I'm here!");

    return 0;
}

MAPSTORE_API json_object *get_data_info(mapstore_ctx *ctx, uint8_t *hash) {
    fprintf(stdout, "I'm here!");
}

MAPSTORE_API json_object *get_store_info(mapstore_ctx *ctx) {
    fprintf(stdout, "I'm here!");;
}
