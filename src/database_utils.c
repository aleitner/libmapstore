#include "database_utils.h"

int prepare_tables(char *database_path) {
    int status = 0;
    char *err_msg = NULL;
    sqlite3 *db = NULL;

    if (sqlite3_open_v2(database_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_prepare_tables;
    }

    char *data_locations_table = "CREATE TABLE IF NOT EXISTS `data_locations` ( "
        "`Id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
        "`hash` TEXT NOT NULL UNIQUE, "
        "`size` INTEGER NOT NULL, "
        "`positions` TEXT NOT NULL, "
        "`uploaded` BOOLEAN NOT NULL )";

    if(sqlite3_exec(db, data_locations_table, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to create table\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        status = 1;
        goto end_prepare_tables;
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
        status = 1;
        goto end_prepare_tables;
    }

    char *mapstore_layout = "CREATE TABLE IF NOT EXISTS `mapstore_layout` ( "
        "`Id` INTEGER NOT NULL PRIMARY KEY, "
        "`map_size` INTEGER NOT NULL, "
        "`allocation_size` INTEGER NOT NULL)";

    if(sqlite3_exec(db, mapstore_layout, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to create table\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        status = 1;
        goto end_prepare_tables;
    }

end_prepare_tables:
    if (db) {
        sqlite3_close(db);
    }

    return status;
}

int get_latest_layout_row(sqlite3 *db, mapstore_layout_row *row) {
    int status = 0;
    int rc;
    char query[BUFSIZ];
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    row->id = 0;
    row->allocation_size = 0;
    row->map_size = 0;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT * FROM `mapstore_layout` ORDER BY Id DESC LIMIT 1");
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_mapstore_layout_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_mapstore_layout_row;
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

end_mapstore_layout_row:
    return status;
}

int get_store_rows(sqlite3 *db, char *where, mapstore_row *row) {
    int status = 0;
    int rc;
    char query[BUFSIZ];
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    row->id = 0;
    row->free_space = 0;
    row->size = 0;
    row->free_locations = NULL;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT * FROM `map_stores` %s LIMIT 1", where);
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_mapstore_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_mapstore_row;
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

end_mapstore_row:
    return status;
}

int get_data_locations_row(sqlite3 *db, char *hash, data_locations_row *row) {
    int status = 0;
    int rc;
    char query[BUFSIZ];
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    row->id = 0;
    row->hash = NULL;
    row->size = 0;
    row->positions = NULL;
    row->uploaded = false;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT * FROM `data_locations` WHERE hash='%s' ORDER BY Id DESC LIMIT 1", hash);
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_data_locations_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_data_locations_row;
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

                        if (strcmp(column_name, "hash") == 0) {
                            row->hash = strdup((const char *)sqlite3_column_text(stmt, i));
                        }

                        if (strcmp(column_name, "size") == 0) {
                            row->size = sqlite3_column_int64(stmt, i);
                        }

                        if (strcmp(column_name, "positions") == 0) {
                            row->positions = json_tokener_parse((const char *)sqlite3_column_text(stmt, i));
                        }

                        if (strcmp(column_name, "uploaded") == 0) {
                            row->uploaded = (strcmp((const char *)sqlite3_column_text(stmt, i), "true") == 0) ? true : false;
                        }
                    }
                }
        }
    }

    sqlite3_finalize(stmt);

end_data_locations_row:
    return status;
}

int sum_column_for_table(sqlite3 *db, char *column, char *table, uint64_t *sum) {
    int status = 0;
    int rc;
    char query[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT SUM(%s) FROM %s;", column, table);
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_mapstore_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_mapstore_row;
            case SQLITE_ROW:
                {
                    *sum = sqlite3_column_int64(stmt, 0);
                }
        }
    }

    sqlite3_finalize(stmt);

end_mapstore_row:
    return status;
}

int update_map_store(sqlite3 *db, char *where, char *set) {
    int status = 0;
    char *err_msg = NULL;

    char *query = NULL;
    asprintf(&query, "UPDATE `map_stores` %s %s LIMIT 1", set, where);

    if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to update map_stores\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        status = 1;
    }

    free(query);

    return status;
}

int insert_to(sqlite3 *db, char *table, char *set) {
    int status = 0;
    char *err_msg = NULL;

    char *query = NULL;
    asprintf(&query, "INSERT INTO `%s` %s", table, set);

    if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to insert into %s\n", table);
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        status = 1;
    }

    free(query);

    return status;
}

int hash_exists_in_mapstore(sqlite3 *db, char *hash) {
    int status = 0;
    int rc;
    char query[BUFSIZ];
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT * FROM `data_locations` WHERE hash='%s' LIMIT 1", hash);
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_mapstore_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_mapstore_row;
            case SQLITE_ROW:
                {
                    int n = sqlite3_column_count(stmt);
                    int i;
                    for (i = 0; i < n; i++) {
                        memset(column_name, '\0', BUFSIZ);
                        strcpy(column_name, sqlite3_column_name(stmt, i));

                        if (strcmp(column_name, "hash") == 0) {
                            if (strcmp((const char *)sqlite3_column_text(stmt, i), (const char *)hash) == 0) {
                                status = 1;
                            };
                        }
                    }
                }
        }
    }

    sqlite3_finalize(stmt);

end_mapstore_row:
    return status;
}

int mark_as_uploaded(sqlite3 *db, char *hash) {
    int status = 0;
    char *err_msg = NULL;

    char *query = NULL;
    asprintf(&query, "UPDATE `data_locations` SET uploaded='true' WHERE hash='%s'", hash);

    if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to update data_locations\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        status = 1;
    }

    free(query);

    return status;
}

int get_pos_from_data_locations(sqlite3 *db, char *hash, json_object **positions) {
    int status = 1;
    int rc;
    char query[BUFSIZ];
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT * FROM `data_locations` WHERE hash='%s' LIMIT 1", hash);
    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        status = 1;
        goto end_mapstore_row;
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                status = 1;
                goto end_mapstore_row;
            case SQLITE_ROW:
                {
                    int n = sqlite3_column_count(stmt);
                    int i;
                    for (i = 0; i < n; i++) {
                        memset(column_name, '\0', BUFSIZ);
                        strcpy(column_name, sqlite3_column_name(stmt, i));

                        if (strcmp(column_name, "positions") == 0) {
                            status = 0;
                            *positions = json_tokener_parse((const char *)sqlite3_column_text(stmt, i));
                        }
                    }
                }
        }
    }

    sqlite3_finalize(stmt);

end_mapstore_row:
    return status;
}

int delete_by_hash_from_data_locations(sqlite3 *db, char *hash) {
    int status = 0;
    char *err_msg = NULL;

    char *query = NULL;
    asprintf(&query, "DELETE FROM `data_locations` WHERE hash='%s'", hash);

    if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to delete hash from data_locations\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        status = 1;
    }

    free(query);

    return status;
}

int delete_by_id_from_map_stores(sqlite3 *db, uint64_t id) {
    int status = 0;
    char *err_msg = NULL;

    char *query = NULL;
    asprintf(&query, "DELETE FROM `map_stores` WHERE Id=%"PRIu64, id);

    if(sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Failed to delete hash from map_stores\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        status = 1;
    }

    free(query);

    return status;
}
