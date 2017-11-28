#include "mapstore.h"

/**
* Initialize everything
*/
MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts) {
    int status = 0;
    sqlite3 *db = NULL;
    ctx->db = NULL;

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

    uint64_t dv = ctx->allocation_size / ctx->map_size; // Number of files to be created except smaller tail file
    uint64_t rm = ctx->allocation_size % ctx->map_size; // Size of smaller tail file
    ctx->total_mapstores = (rm > 0) ? dv + 1 : dv;   // If there is a remainder make sure we create a row an map for that smaller file

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

    if (sqlite3_open_v2(
        ctx->database_path,
        &db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_initalize;
    }

    ctx->db = db;

    /* All variables have been initialized for the ctx by now */

    /* Prepare mapstore tables */
    if (prepare_tables(ctx->db) != 0) {
        fprintf(stderr, "Could not create tables\n");
        status = 1;
        goto end_initalize;
    };

    /* get previous layout for comparing size changes */
    mapstore_layout_row previous_layout;
    if (get_latest_layout_row(ctx->db, &previous_layout) != 0) {
        goto end_initalize;
    };

    char mapstore_path[BUFSIZ];         // Path to map_store
    char query[BUFSIZ];                 // SQL query
    char *err_msg = NULL;               // error from sqlite3_exec
    json_object *json_positions = NULL; // Positions of free space

    /* Insert table layout. We keep track of the change over time so no need to delete old rows */
    memset(query, '\0', BUFSIZ);
    sprintf(query,
            "(map_size,allocation_size) VALUES(%"PRIu64",%"PRIu64")",
            ctx->map_size,
            ctx->allocation_size);

    char *mapstore_layout = "mapstore_layout";
    if (insert_to(db, mapstore_layout, query) != 0) {
        status = 1;
        goto end_initalize;
    }

    /* Update database to know metadata about each mmap file */
    mapstore_row row;       // Previous Mapstore information
    uint64_t map_size;      // Size of mapstore allocation
    uint64_t free_space;    // Free space for mapstore
    uint64_t id;            // Mapstore Id

    for (uint64_t f = 1; f <= ctx->total_mapstores; f++) {

        /* Check if mapstore already exists */
        char where[BUFSIZ];
        memset(where, '\0', BUFSIZ);
        sprintf(where, "WHERE Id = %"PRIu64, f);
        if (get_store_rows(db, where, &row) != 0) {
            status = 1;
            goto end_initalize;
        };

        // Adjust new map sizes and free space if larger allocations
        if (ctx->allocation_size > previous_layout.allocation_size && row.size < ctx->map_size) {
            if (f > dv) {
                map_size = rm; // Last store might be smaller than the rest
                free_space = rm - row.size + row.free_space;
            } else {
                map_size = ctx->map_size;
                free_space = ctx->map_size - row.size + row.free_space;
            }
        } else {
            continue;
        }

        /* Delete old data if new data is coming in*/
        if (row.free_locations) {
            memset(query, '\0', BUFSIZ);
            sprintf(query, "DELETE from map_stores WHERE Id=%"PRIu64";", f);
            if (delete_by_id_from_map_stores(db, f) != 0) {
                status = 1;
                goto end_initalize;
            }
        }

        /* Insert new data */
        memset(query, '\0', BUFSIZ);
        json_positions = expand_free_space_list(row.free_locations, row.size, map_size);
        sprintf(query,
                "VALUES(%"PRIu64", '%s', %"PRIu64", %"PRIu64")",
                f,
                json_object_to_json_string(json_positions),
                free_space,
                map_size);

        json_object_put(json_positions);

        char *map_stores = "map_stores";
        if (insert_to(db, map_stores, query) != 0) {
            status = 1;
            goto end_initalize;
        }

        // Create map file
        // Only increase map size. Never decrease.
        if (ctx->allocation_size > previous_layout.allocation_size && row.size < map_size) {
            memset(mapstore_path, '\0', BUFSIZ);
            sprintf(mapstore_path, "%s%"PRIu64".map", ctx->mapstore_path, f);
            if (create_map_store(mapstore_path, map_size) != 0) {
                fprintf(stderr,
                    "Failed to create mapped file: %s of size %"PRIu64,
                    ctx->mapstore_path,
                    map_size);

                status = 1;
                goto end_initalize;
            };
        }
    }

    // Make sure we have stores
    struct stat st;
    for (uint64_t store = 1; store <= ctx->total_mapstores; store++) {
        memset(mapstore_path, '\0', BUFSIZ);
        sprintf(mapstore_path, "%s%"PRIu64".map", ctx->mapstore_path, store);
        if (stat(mapstore_path, &st) != 0) {
            status = 1;
            goto end_initalize;
        }
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
MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, char *hash) {
    int status = 0;
    json_object *map_plan = json_object_new_object();
    char where[BUFSIZ];
    char set[BUFSIZ];
    json_object *free_pos_obj = NULL;
    json_object *store_positions_obj = NULL;
    json_object *used_space_obj = NULL;
    json_object *all_data_locations = json_object_new_object();

    if((status = hash_exists_in_mapstore(ctx->db, hash)) != 0) {
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
    if((status = get_map_plan(ctx->db, ctx->total_mapstores, data_size, map_plan)) != 0) {
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

        if((status = update_map_store(ctx->db, where, set)) != 0) {
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
    if((status = insert_to(ctx->db, table, set)) != 0) {
        status = 1;
        goto end_store_data;
    }

    // Store data in mmap files
    if((status = write_to_store(fd, ctx->mapstore_path, all_data_locations)) != 0) {
        status = 1;
        goto end_store_data;
    }

    // Set uploaded to true in data_locations
    if((status = mark_as_uploaded(ctx->db, hash)) != 0) {
        status = 1;
        goto end_store_data;
    }

end_store_data:
    if (map_plan) {
        json_object_put(map_plan);
    }
    return status;
}

/**
* Retrieve data
*/
MAPSTORE_API int retrieve_data(mapstore_ctx *ctx, int fd, char *hash) {
    int status = 0;
    json_object *positions = NULL;

    // get data map
    if ((status = get_pos_from_data_locations(ctx->db, hash, &positions)) != 0) {
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
    json_object *positions = NULL;
    json_object *updated_positions = NULL;
    json_object *free_pos_obj = NULL;
    json_object *free_space_obj = NULL;
    char where[BUFSIZ];
    char set[BUFSIZ];

    // get data map
    if ((status = get_pos_from_data_locations(ctx->db, hash, &positions)) != 0) {
        fprintf(stderr, "Failed to get positions from data_locations table\n");
        status = 1;
        goto end_delete_data;
    }

    // add each location to map_stores table free_locations
    if ((status = get_updated_free_locations(ctx->db, positions, &updated_positions)) != 0) {
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

        if((status = update_map_store(ctx->db, where, set)) != 0) {
            status = 1;
            goto end_delete_data;
        }
    }

    // Delete data_locations row by hash
    if((status = delete_by_hash_from_data_locations(ctx->db, hash)) != 0) {
        status = 1;
        goto end_delete_data;
    }

end_delete_data:
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
