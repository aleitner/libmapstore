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

    // If sqlite doesn't exist create below
    struct stat st = {0};

    if (stat(ctx->database_path, &st) == -1) {
        fprintf(stdout, "Preparing database\n");

        if (prepare_tables(ctx) != 0) {
            fprintf(stderr, "Could not create tables\n");
            status = 1;
            goto end_initalize;
        };

        if (map_files(ctx) != 0) {
            fprintf(stderr, "Could not create tables\n");
            status = 1;
            goto end_initalize;
        };
    }

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

//////////////////////
// Static functions //
//////////////////////

static int prepare_tables(mapstore_ctx *ctx) {
    int status = 0;
    char *err_msg = NULL;
    sqlite3 *db = NULL;
    char *database_path = ctx->database_path;

    if (sqlite3_open(database_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_prepare_tables;
    }

    char *data_locations_table = "CREATE TABLE IF NOT EXISTS `data_locations` ( "
        "`Id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
        "`hash` TEXT NOT NULL UNIQUE, "
        "`size` INTEGER NOT NULL, "
        "`positions` TEXT NOT NULL, "
        "`date` NUMERIC NOT NULL )";

    if(sqlite3_exec(db, data_locations_table, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to create table\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    char *map_stores = "CREATE TABLE IF NOT EXISTS `map_stores` ( "
        "`Id` INTEGER NOT NULL, "
        "`free_locations` TEXT NOT NULL, "
        "`free_space` INTEGER NOT NULL)";

    if(sqlite3_exec(db, map_stores, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to create table\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        goto end_prepare_tables;
    }

end_prepare_tables:
    if (db) {
        sqlite3_close(db);
    }

    return status;
}

static json_object *create_json_positions_array(uint64_t start, uint64_t end) {
    json_object *jarray = json_object_new_array();
    json_object *first = json_object_new_int64(start);
    json_object *final = json_object_new_int64(end);
    json_object_array_add(jarray,first);
    json_object_array_add(jarray,final);

    return jarray;
}

static int map_files(mapstore_ctx *ctx) {
    int status = 0;
    uint64_t dv = ctx->allocation_size / ctx->map_size;
    uint64_t rm = ctx->allocation_size % ctx->map_size;

    int f = 0;
    char query[BUFSIZ];
    sqlite3 *db = NULL;
    char *err_msg = NULL;
    json_object *json_positions = NULL;

    if (sqlite3_open(ctx->database_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_map_files;
    }

    fprintf(stdout, "%llu files of size %llu\n", dv, ctx->map_size);
    for (f = 1; f < dv + 1; f++) {
        memset(query, '\0', BUFSIZ);
        json_positions = create_json_positions_array(0, ctx->map_size - 1);
        sprintf(query, "INSERT INTO `map_stores` VALUES(%d, '%s', %llu);", f, json_object_to_json_string(json_positions), ctx->map_size);
        if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "Failed to create table\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            goto end_map_files;
        }

        // Create map file
    }

    if (rm > 0) {
        fprintf(stdout, "1 file of size %llu\n", rm);
        memset(query, '\0', BUFSIZ);
        json_positions = create_json_positions_array(0, rm - 1);
        sprintf(query, "INSERT INTO `map_stores` VALUES(%d, '%s', %llu);", f+1, json_object_to_json_string(json_positions), rm);
        if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "Failed to create table\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            goto end_map_files;
        }
    }

end_map_files:
    if (db) {
        sqlite3_close(db);
    }

    return status;
}
