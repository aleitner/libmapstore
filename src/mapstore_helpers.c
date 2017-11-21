#include "mapstore.h"

int get_map_plan(sqlite3 *db,
                        uint64_t total_stores,
                        uint64_t data_size,
                        json_object *map_coordinates) {
    int status = 0;
    char where[BUFSIZ];

    // Determine space available 1:
    uint64_t total_free_space = 0;
    if ((status = sum_column_for_table(db, "free_space", "map_stores", &total_free_space)) != 0) {
        goto end_map_plan;
    }

    mapstore_row row;
    uint64_t remaining = data_size;
    uint64_t min = 0;
    // Get info for each map store

    for (uint64_t f = 1; f <= total_stores; f++) {
        uint64_t min = (remaining > sector_min(data_size)) ? sector_min(data_size) : remaining;
        memset(where, '\0', BUFSIZ);
        sprintf(where, "WHERE Id = %"PRIu64" AND free_space > %"PRIu64, f, min);

        if (get_store_rows(db, where, &row) != 0) {
            status = 1;
            goto end_map_plan;
        };

        // If row doesn't have enough free space don't use it.
        if (remaining <= 0) {
            break;
        }

        // If row is empty
        if (row.free_space <= 0) {
            continue;
        }
        remaining -= prepare_store_positions(f,
                                             row.free_locations,
                                             data_size - remaining,
                                             remaining,
                                             map_coordinates);
    }

    if (remaining > 0 ) {
        fprintf(stderr, "Not free enough space in mapstore\n");
        status = 1;
        goto end_map_plan;
    }

end_map_plan:
    return status;
}

int map_files(mapstore_ctx *ctx) {
    int status = 0;

    uint64_t dv = ctx->allocation_size / ctx->map_size; // NUmber of files to be created except smaller tail file
    uint64_t rm = ctx->allocation_size % ctx->map_size; // Size of smaller tail file

    char mapstore_path[BUFSIZ];         // Path to map_store
    char query[BUFSIZ];                 // SQL query
    sqlite3 *db = ctx->db;                 // Database
    char *err_msg = NULL;               // error from sqlite3_exec
    json_object *json_positions = NULL; // Positions of free space

    /* get previous layout for comparing size changes */
    mapstore_layout_row previous_layout;
    if (get_latest_layout_row(db, &previous_layout) != 0) {
        goto end_map_files;
    };

    /* Insert table layout. We keep track of the change over time so no need to delete old rows */
    memset(query, '\0', BUFSIZ);
    sprintf(query,
            "(map_size,allocation_size) VALUES(%"PRIu64",%"PRIu64")",
            ctx->map_size,
            ctx->allocation_size);

    char *mapstore_layout = "mapstore_layout";
    if (insert_to(db, mapstore_layout, query) != 0) {
        status = 1;
        goto end_map_files;
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
            if (delete_by_id_from_map_stores(db, f) != 0) {
                status = 1;
                goto end_map_files;
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
            goto end_map_files;
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
                goto end_map_files;
            };
        }
    }

end_map_files:
    return status;
}

int get_updated_free_locations(sqlite3 *db, json_object *positions, json_object **updated_positions) {
    int status = 0;
    char where[BUFSIZ];
    mapstore_row row;
    json_object *location_array = NULL;
    int arr_i = 0;
    json_object *free_locations = NULL;
    uint64_t first = 0;
    uint64_t last = 0;
    json_object *map_store_obj = NULL;
    json_object *free_positions = NULL;
    uint64_t freespace = 0;

    *updated_positions = json_object_new_object();

    json_object_object_foreach(positions, store_id, pos) {
        memset(where, '\0', BUFSIZ);
        sprintf(where, "WHERE Id = %s", store_id);

        if (get_store_rows(db, where, &row) != 0) {
            status = 1;
            goto end_update_free_locations;
        };


        // Combine used locations with free locations
        free_locations = json_object_new_array();

        for (arr_i = 0; arr_i < json_object_array_length(row.free_locations); arr_i++) {
            json_object_array_add(free_locations, json_object_array_get_idx(row.free_locations, arr_i));

        }

        for (arr_i = 0; arr_i < json_object_array_length(pos); arr_i++) {
            location_array = json_object_array_get_idx(pos, arr_i);
            first = json_object_get_int64(json_object_array_get_idx(location_array, 1));
            last = json_object_get_int64(json_object_array_get_idx(location_array, 2));
            json_object_array_add(free_locations, json_free_space_array(first, last));
        }

        // SORT and combine locations
        freespace = 0;
        map_store_obj = json_object_new_object();
        free_positions = combine_positions(free_locations, &freespace);
        json_object_object_add(map_store_obj, "free_positions", free_positions);
        json_object_object_add(map_store_obj, "free_space", json_object_new_int64(freespace));
        json_object_object_add(*updated_positions, store_id, map_store_obj);

    }

end_update_free_locations:
    return status;
}
