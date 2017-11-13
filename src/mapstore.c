#include "mapstore.h"

/**
* Initialize everything
*/
MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts) {
    int status = 0;

    /* Initialized path variables */
    ctx->mapstore_path = NULL;
    ctx->database_path = NULL;

    /* Allocation size is required */
    if (!opts.allocation_size) {
        fprintf(stderr, "Can't initialize mapstore context. \
                Missing required allocation size\n");
        status = 1;
        goto end_initalize;
    }

    ctx->allocation_size = opts.allocation_size;
    ctx->map_size = (opts.map_size) ? opts.map_size : 2147483648; // Default to 2GB if not map_size provided

    /* Format path for shardata and database n*/
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

    /* Create map store folder */
    if (create_directory(ctx->mapstore_path) != 0) {
        fprintf(stderr, "Could not create folder: %s\n", ctx->mapstore_path);
        status = 1;
        goto end_initalize;
    };

    ctx->database_path = calloc(strlen(base_path) + 15, sizeof(char));
    sprintf(ctx->database_path, "%s%cshards.sqlite", base_path, separator());

    /* Prepare mapstore tables */
    if (prepare_tables(ctx->database_path) != 0) {
        fprintf(stderr, "Could not create tables\n");
        status = 1;
        goto end_initalize;
    };

    /* Prepare mapstore files and database entries for each file */
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
MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, char *hash) {
    int status = 0;
    json_object *map_plan = json_object_new_object();
    sqlite3 *db = NULL;
    char where[BUFSIZ];
    char set[BUFSIZ];
    json_object *free_pos_obj = NULL;
    json_object *store_positions_obj = NULL;
    json_object *used_space_obj = NULL;
    json_object *all_data_locations = json_object_new_object();

    if (sqlite3_open_v2(ctx->database_path, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_store_data;
    }

    if((status = hash_exists_in_mapstore(db, hash)) != 0) {
        fprintf(stderr, "Hash already exists in mapstore\n");
        status = 1;
        goto end_store_data;
    }

    uint64_t data_size = get_file_size(fd);
    if (data_size <= 0) {
        status = 1;
        goto end_store_data;
    }

    // Determine space available
    if((status = get_map_plan(db, ctx->total_mapstores, data_size, map_plan)) != 0) {
        status = 1;
        goto end_store_data;
    }

    // Update map_stores free_locations and free_space
    json_object_object_foreach(map_plan, store_id, store_meta) {
        json_object_object_get_ex(store_meta, "free_positions", &free_pos_obj);
        json_object_object_get_ex(store_meta, "store_positions", &store_positions_obj);
        json_object_object_get_ex(store_meta, "used_space", &used_space_obj);
        memset(where, '\0', BUFSIZ);
        memset(set, '\0', BUFSIZ);
        sprintf(where, "WHERE Id=%s", store_id);
        sprintf(set,
                "SET free_space = free_space - %"PRIu64", free_locations = '%s'",
                json_object_get_int64(used_space_obj),
                json_object_to_json_string(free_pos_obj));

        if((status = update_map_store(db, where, set)) != 0) {
            status = 1;
            goto end_store_data;
        }

        json_object_object_add(all_data_locations, store_id, store_positions_obj);
    }

    // Add file to data_locations
    memset(set, '\0', BUFSIZ);
    sprintf(set,
            "(hash,size,positions,uploaded) VALUES('%s',%"PRIu64",'%s','false')",
            hash,
            data_size,
            json_object_to_json_string(all_data_locations));

    char *table = "data_locations";
    if((status = insert_to(db, table, set)) != 0) {
        status = 1;
        goto end_store_data;
    }

    // Store data in mmap files
    if((status = write_to_store(fd, ctx->mapstore_path, all_data_locations)) != 0) {
        status = 1;
        goto end_store_data;
    }

    // Set uploaded to true in data_locations
    if((status = mark_as_uploaded(db, hash)) != 0) {
        status = 1;
        goto end_store_data;
    }

end_store_data:
    if (map_plan) {
        json_object_put(map_plan);
    }

    if (db) {
        sqlite3_close(db);
    }

    return status;
}

/**
* Retrieve data
*/
MAPSTORE_API int retrieve_data(mapstore_ctx *ctx, int fd, char *hash) {
    int status = 0;
    sqlite3 *db = NULL;                 // Database
    json_object *positions = NULL;

    /* Open Database */
    if (sqlite3_open_v2(ctx->database_path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_retrieve_data;
    }

    // get data map
    if ((status = get_pos_from_data_locations(db, hash, &positions)) != 0) {
        fprintf(stderr, "Failed to get positions from data_locations table\n");
        status = 1;
        goto end_retrieve_data;
    }

    // read from files according to data maps
    if((status = read_from_store(fd, ctx->mapstore_path, positions)) != 0) {
        fprintf(stderr, "Failed to get retreive data from store\n");
        status = 1;
        goto end_retrieve_data;
    }

end_retrieve_data:
    if (db) {
        sqlite3_close(db);
    }

    if (positions) {
        json_object_put(positions);
    }
    return status;
}

/**
* Delete data
*/
MAPSTORE_API int delete_data(mapstore_ctx *ctx, char *hash) {
    int status = 0;
    sqlite3 *db = NULL;                 // Database
    json_object *positions = NULL;
    json_object *updated_positions = NULL;
    json_object *free_pos_obj = NULL;
    json_object *free_space_obj = NULL;
    char where[BUFSIZ];
    char set[BUFSIZ];

    /* Open Database */
    if (sqlite3_open_v2(ctx->database_path, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_delete_data;
    }

    // get data map
    if ((status = get_pos_from_data_locations(db, hash, &positions)) != 0) {
        fprintf(stderr, "Failed to get positions from data_locations table\n");
        status = 1;
        goto end_delete_data;
    }

    // add each location to map_stores table free_locations
    if ((status = get_updated_free_locations(db, positions, &updated_positions)) != 0) {
        status = 1;
        goto end_delete_data;
    };

    // Update freespace for map_store with updated_positions
    json_object_object_foreach(updated_positions, store_id, store_meta) {
        json_object_object_get_ex(store_meta, "free_positions", &free_pos_obj);
        json_object_object_get_ex(store_meta, "free_space", &free_space_obj);
        memset(where, '\0', BUFSIZ);
        memset(set, '\0', BUFSIZ);
        sprintf(where, "WHERE Id=%s", store_id);
        sprintf(set,
                "SET free_space = %"PRIu64", free_locations = '%s'",
                json_object_get_int64(free_space_obj),
                json_object_to_json_string(free_pos_obj));

        if((status = update_map_store(db, where, set)) != 0) {
            status = 1;
            goto end_delete_data;
        }
    }

    // Delete data_locations row by hash
    if((status = delete_by_hash_from_data_locations(db, hash)) != 0) {
        status = 1;
        goto end_delete_data;
    }
    // Empty mapstore positions

end_delete_data:
    if (db) {
        sqlite3_close(db);
    }

    if (positions) {
        json_object_put(positions);
    }
    return status;
}

MAPSTORE_API data_info *get_data_info(mapstore_ctx *ctx, char *hash) {
    fprintf(stdout, "I'm here!");
}

MAPSTORE_API store_info *get_store_info(mapstore_ctx *ctx) {
    fprintf(stdout, "I'm here!");;
}
