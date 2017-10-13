#include "database_utils.h"

int prepare_tables(char *database_path) {
    int status = 0;
    char *err_msg = NULL;
    sqlite3 *db = NULL;

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

                        if (strcmp(column_name, "Id") == 0) {
                            row->id = sqlite3_column_int64(stmt, i);
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

int get_store_row_by_id(sqlite3 *db, uint64_t id, map_store_row *row) {
    int status = 0;
    int rc;
    char query[BUFSIZ];
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT * FROM `map_stores` WHERE Id = %llu LIMIT 1", id);
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_map_store_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_map_store_row;
            case SQLITE_ROW:
                {
                    int n = sqlite3_column_count(stmt);
                    int i;
                    for (i = 0; i < n; i++) {
                        memset(column_name, '\0', BUFSIZ);
                        strcpy(column_name, sqlite3_column_name(stmt, i));

                        if (strcmp(column_name, "Id") == 0) {
                            row->id = sqlite3_column_int64(stmt, i);
                        }

                        if (strcmp(column_name, "free_space") == 0) {
                            row->free_space = sqlite3_column_int64(stmt, i);
                        }

                        if (strcmp(column_name, "size") == 0) {
                            row->size = sqlite3_column_int64(stmt, i);
                        }

                        if (strcmp(column_name, "free_locations") == 0) {
                            row->free_locations = json_tokener_parse((const char *)sqlite3_column_text(stmt, i));
                        }
                    }
                }
        }
    }

    sqlite3_finalize(stmt);

end_map_store_row:
    return status;
}
