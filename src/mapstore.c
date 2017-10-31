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
MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, uint8_t *hash) {
    fprintf(stdout, "Begin Store Data\n");
    int status = 0;
    json_object *map_coordinates = NULL;

    uint64_t data_size = get_file_size(fd);
    if (data_size <= 0) {
        status = 1;
        goto end_store_data;
    }

    map_coordinates = json_object_new_object(); // All Locations to store data in map store
    // Determine space available

    if((status = get_map_plan(ctx, data_size, map_coordinates)) != 0) {
        status = 1;
        goto end_store_data;
    }

    printf("map_coordinates: %s\n", json_object_to_json_string(map_coordinates));

    // Update map_stores free_locations and free_space
    // Add file to data_locations
    // Store data in mmap files

    printf("Data size: %"PRIu64"\n", data_size);

    printf("Status: %d\n", status);

end_store_data:
    if (map_coordinates) {
        json_object_put(map_coordinates);
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

MAPSTORE_API data_info *get_data_info(mapstore_ctx *ctx, uint8_t *hash) {
    fprintf(stdout, "I'm here!");
}

MAPSTORE_API store_info *get_store_info(mapstore_ctx *ctx) {
    fprintf(stdout, "I'm here!");;
}

static int get_map_plan(mapstore_ctx *ctx, uint64_t data_size, json_object *map_coordinates) {
    int status = 0;
    sqlite3 *db = NULL;
    if (sqlite3_open(ctx->database_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_map_plan;
    }

    // Determine space available 1:
    uint64_t total_free_space = 0;
    if ((status = sum_column_for_table(db, "free_space", "map_stores", &total_free_space)) != 0) {
        goto end_map_plan;
    }

    mapstore_row row;
    uint64_t remaining = data_size;
    uint64_t min = 0;
    // Get info for each map store
    char where[BUFSIZ];
    for (uint64_t f = 1; f < ctx->total_mapstores; f++) {
        uint64_t min = (remaining > sector_min(data_size)) ? sector_min(data_size) : remaining;
        memset(where, '\0', BUFSIZ);
        sprintf(where, "WHERE Id = %"PRIu64" AND free_space > %"PRIu64, f, min);
        
        if (get_store_rows(db, where, &row) != 0) {
            status = 1;
            goto end_map_plan;
        };

        // If row doesn't have enough free space don't use it.
        remaining -= prepare_store_positions(f, row.free_locations, remaining, map_coordinates);

    }

    if (remaining > 0 ) {
        printf("Not enough space\n");
        status = 1;
        goto end_map_plan;
    }

end_map_plan:
    if (db) {
        sqlite3_close(db);
    }

    return status;
}

static int map_files(mapstore_ctx *ctx) {
    /** TODO: Separate database calls into separate database_utils
    *         functions so we can test better and replace with
    *         another database if sqlite doesn't work out
    */

    int status = 0;

    uint64_t dv = ctx->allocation_size / ctx->map_size; // NUmber of files to be created except smaller tail file
    uint64_t rm = ctx->allocation_size % ctx->map_size; // Size of smaller tail file
    ctx->total_mapstores = (rm > 0) ? dv + 2 : dv + 1;   // If there is a remainder make sure we create a row an map for that smaller file

    char mapstore_path[BUFSIZ];         // Path to map_store
    char query[BUFSIZ];                 // SQL query
    sqlite3 *db = NULL;                 // Database
    char *err_msg = NULL;               // error from sqlite3_exec
    json_object *json_positions = NULL; // Positions of free space

    /* Open Database */
    if (sqlite3_open(ctx->database_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_map_files;
    }

    /* get previous layout for comparing size changes */
    mapstore_layout_row previous_layout;
    if (get_latest_layout_row(db, &previous_layout) != 0) {
        goto end_map_files;
    };

    /* Insert table layout. We keep track of the change over time so no need to delete old rows */
    memset(query, '\0', BUFSIZ);
    sprintf(query, "INSERT INTO `mapstore_layout` (map_size,allocation_size) VALUES(%"PRIu64",%"PRIu64");", ctx->map_size, ctx->allocation_size);
    if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to create mapstore_layout\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        goto end_map_files;
    }

    fprintf(stdout, "%"PRIu64" files of size %"PRIu64"\n", dv, ctx->map_size);

    /* Update database to know metadata about each mmap file */
    mapstore_row row;       // Previous Mapstore information
    uint64_t map_size;      // Size of mapstore allocation
    uint64_t free_space;    // Free space for mapstore
    uint64_t id;            // Mapstore Id

    for (uint64_t f = 1; f < ctx->total_mapstores; f++) {

        /* Check if mapstore already exists */
        char where[BUFSIZ];
        memset(where, '\0', BUFSIZ);
        sprintf(where, "WHERE Id = %"PRIu64, f);
        if (get_store_rows(db, where, &row) != 0) {
            status = 1;
            goto end_map_files;
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
            map_size = row.size;
            free_space = row.free_space;
        }

        /* Delete old data if new data is coming in*/
        if (row.free_locations) {
            memset(query, '\0', BUFSIZ);
            sprintf(query, "DELETE from map_stores WHERE Id=%"PRIu64";", f);
            if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
                fprintf(stderr, "Failed to delete from table map_stores\n");
                fprintf(stderr, "SQL error: %s\n", err_msg);
                sqlite3_free(err_msg);
                goto end_map_files;
            }
        }

        /* Insert new data */
        memset(query, '\0', BUFSIZ);
        json_positions = expand_free_space_list(row.free_locations, row.size, map_size);
        sprintf(query, "INSERT INTO `map_stores` VALUES(%"PRIu64", '%s', %"PRIu64", %"PRIu64");", f, json_object_to_json_string(json_positions), free_space, map_size);

        if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "Failed to insert to table map_stores\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            json_object_put(json_positions);
            goto end_map_files;
        }

        json_object_put(json_positions);

        // Create map file
        // Only increase map size. Never decrease.
        if (ctx->allocation_size > previous_layout.allocation_size && row.size < map_size) {
            memset(mapstore_path, '\0', BUFSIZ);
            sprintf(mapstore_path, "%s%"PRIu64".map", ctx->mapstore_path, f);
            if (create_map_store(mapstore_path, map_size) != 0) {
                fprintf(stderr, "Failed to create mapped file: %s of size %"PRIu64"", ctx->mapstore_path, map_size);
                status = 1;
                goto end_map_files;
            };
        }
    }

end_map_files:
    if (db) {
        sqlite3_close(db);
    }

    return status;
}
