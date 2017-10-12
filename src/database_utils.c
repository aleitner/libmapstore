#include "database_utils.h"

int prepare_tables(mapstore_ctx *ctx) {
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
        "`free_space` INTEGER NOT NULL, "
        "`size` INTEGER NOT NULL)";

    if(sqlite3_exec(db, map_stores, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to create table\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        goto end_prepare_tables;
    }

    char *map_store_layout = "CREATE TABLE IF NOT EXISTS `map_store_layout` ( "
        "`Id` INTEGER NOT NULL PRIMARY KEY, "
        "`map_size` INTEGER NOT NULL, "
        "`allocation_size` INTEGER NOT NULL)";

    if(sqlite3_exec(db, map_store_layout, 0, 0, &err_msg) != SQLITE_OK) {
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

int map_files(mapstore_ctx *ctx) {
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
    for (f = 1; f < dv + 1; f++) {

        if (ctx->allocation_size > previous_layout.allocation_size) {
            printf("New allocation size.\n");
        }
        // Check if alloc size increased
        // Check if file_size needs to be increased
        // If so increase freespace with difference of increased size and last free space
        // add free location of previous [file_size, new size -1]
        // Update

        // ELSE

        memset(query, '\0', BUFSIZ);
        json_positions = create_json_positions_array(0, ctx->map_size - 1);
        sprintf(query, "INSERT INTO `map_stores` VALUES(%d, '%s', %llu, %llu);", f, json_object_to_json_string(json_positions), ctx->map_size, ctx->map_size);
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
        fprintf(stdout, "1 file of size %llu\n", rm);
        memset(query, '\0', BUFSIZ);
        json_positions = create_json_positions_array(0, rm - 1);
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


int get_latest_layout_row(sqlite3 *db, map_store_layout_row *row) {
    int status = 0;
    int rc;
    char query[BUFSIZ];
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT * FROM `map_store_layout` ORDER BY Id DESC LIMIT 1");
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_map_store_layout_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_map_store_layout_row;
            case SQLITE_ROW:
                {
                    int n = sqlite3_column_count(stmt);
                    int i;
                    for (i = 0; i < n; i++) {
                        memset(column_name, '\0', BUFSIZ);
                        strcpy(column_name, sqlite3_column_name(stmt, i));

                        if (strcmp(column_name, "primary_key") == 0) {
                            row->primary_key = sqlite3_column_int64(stmt, i);
                        }

                        if (strcmp(column_name, "allocation_size") == 0) {
                            row->allocation_size = sqlite3_column_int64(stmt, i);
                        }

                        if (strcmp(column_name, "map_size") == 0) {
                            row->map_size = sqlite3_column_int64(stmt, i);
                        }
                    }
                }
        }
    }

    sqlite3_finalize(stmt);

end_map_store_layout_row:
    return status;
}
