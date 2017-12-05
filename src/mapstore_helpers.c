#include "mapstore.h"

int get_map_plan(sqlite3 *db,
                        uint64_t total_stores,
                        uint64_t data_size,
                        json_object *map_coordinates) {
    int status = 0;
    int where_len = 31 + MAX_UINT64_STR + MAX_UINT64_STR + 1;
    char where[where_len];

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
        memset(where, '\0', where_len);
        sprintf(where, "WHERE Id = %"PRIu64" AND free_space > %"PRIu64, f, min);

        if (get_store_rows(db, where, &row) != 0) {
            status = 1;
            goto end_map_plan;
        };

        // If row doesn't have enough free space don't use it.
        if (remaining <= 0) {
            if (row.free_locations) {
                json_object_put(row.free_locations);
            }
            break;
        }

        // If row is empty
        if (row.free_space <= 0) {
            if (row.free_locations) {
                json_object_put(row.free_locations);
            }
            continue;
        }
        remaining -= prepare_store_positions(f,
                                             row.free_locations,
                                             data_size - remaining,
                                             remaining,
                                             map_coordinates);

        json_object_put(row.free_locations);
    }

    if (remaining > 0 ) {
        fprintf(stderr, "Not free enough space in mapstore\n");
        status = 1;
        goto end_map_plan;
    }

end_map_plan:
    return status;
}

int get_updated_free_locations(sqlite3 *db, json_object *positions, json_object **updated_positions) {
    int status = 0;
    char where[11 + MAX_UINT64_STR + 1];
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
        memset(where, '\0', 11 + MAX_UINT64_STR + 1);
        sprintf(where, "WHERE Id = %s", store_id);

        if (get_store_rows(db, where, &row) != 0) {
            status = 1;
            json_object_put(*updated_positions);
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

        json_object_put(row.free_locations);
    }

end_update_free_locations:
    return status;
}
