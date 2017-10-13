#include "mapstore.h"
#include "utils.h"
#include "database_utils.h"

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

    if (prepare_tables(ctx->database_path) != 0) {
        fprintf(stderr, "Could not create tables\n");
        status = 1;
        goto end_initalize;
    };

    if (map_files(ctx) != 0) {
        fprintf(stderr, "Could not create mmap files\n");
        status = 1;
        goto end_initalize;
    };

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

static int map_files(mapstore_ctx *ctx) {
    int status = 0;
    uint64_t dv = ctx->allocation_size / ctx->map_size;
    uint64_t rm = ctx->allocation_size % ctx->map_size;

    int f = 0;                          // File name and id in database
    char query[BUFSIZ];                 // SQL query
    sqlite3 *db = NULL;                 // Database
    char *err_msg = NULL;               // error from sqlite3_exec
    json_object *json_positions = NULL; // Positions of free space

    if (sqlite3_open(ctx->database_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_map_files;
    }

    /* get previous layout */
    map_store_layout_row previous_layout;
    if (get_latest_layout_row(db, &previous_layout) != 0) {
        goto end_map_files;
    };


    /* Insert table layout. We can keep track of the change over time */
    memset(query, '\0', BUFSIZ);
    sprintf(query, "INSERT INTO `map_store_layout` (map_size,allocation_size) VALUES(%llu,%llu);", ctx->map_size, ctx->allocation_size);
    if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to create map_store_layout\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        goto end_map_files;
    }

    /* Update database to know metadata about each mmap file */
    fprintf(stdout, "%llu files of size %llu\n", dv, ctx->map_size);
    map_store_row row;
    uint64_t map_size = (ctx->map_size > previous_layout.map_size) ? ctx->map_size : previous_layout.map_size;
    uint64_t free_space = ctx->map_size;
    uint64_t id;
    for (f = 1; f < dv + 1; f++) {

        if (get_store_row_by_id(db, f, &row) != 0) {
            goto end_map_files;
        };

        // Adjust new sizes if larger allocations
        if (ctx->allocation_size > previous_layout.allocation_size) {
            printf("New allocation size.\n");
            if (row.size < ctx->map_size) {
                free_space = ctx->map_size - row.size + row.free_space;
            }

        }

        /* Delete old data if new data is coming in*/
        if (row.free_locations) {
            memset(query, '\0', BUFSIZ);
            sprintf(query, "DELETE from map_stores WHERE Id=%d;", f);
            if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
                fprintf(stderr, "Failed to delete from table map_stores\n");
                fprintf(stderr, "SQL error: %s\n", err_msg);
                sqlite3_free(err_msg);
                goto end_map_files;
            }
        }

        /* Insert new data */
        memset(query, '\0', BUFSIZ);
        json_positions = expand_free_space_list(row.free_locations, row.size, ctx->map_size);
        sprintf(query, "INSERT INTO `map_stores` VALUES(%d, '%s', %llu, %llu);", f, json_object_to_json_string(json_positions), free_space, map_size);
        if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "Failed to insert to table map_stores\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            json_object_put(json_positions);
            goto end_map_files;
        }
        json_object_put(json_positions);
        // Create map file
    }

    /* Update database to know metadata if last mmap file is smaller than rest */
    if (rm > 0) {
        if (get_store_row_by_id(db, f+1, &row) != 0) {
            goto end_map_files;
        };

        /* Delete old data if new data is coming in*/
        if (row.free_locations) {
            memset(query, '\0', BUFSIZ);
            sprintf(query, "DELETE from map_stores WHERE Id=%d;", f+1);
            if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
                fprintf(stderr, "Failed to delete from table map_stores\n");
                fprintf(stderr, "SQL error: %s\n", err_msg);
                sqlite3_free(err_msg);
                json_object_put(json_positions);
                goto end_map_files;
            }
        }

        fprintf(stdout, "1 file of size %llu\n", rm);
        memset(query, '\0', BUFSIZ);
        json_positions = expand_free_space_list(row.free_locations, row.size, rm);
        sprintf(query, "INSERT INTO `map_stores` VALUES(%d, '%s', %llu, %llu);", f+1, json_object_to_json_string(json_positions), rm, rm);
        if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "Failed to insert to table map_stores\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            json_object_put(json_positions);
            goto end_map_files;
        }
        json_object_put(json_positions);
    }

end_map_files:
    if (db) {
        sqlite3_close(db);
    }

    return status;
}
