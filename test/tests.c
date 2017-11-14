#include "tests.h"

char *folder = NULL;
char test_case[BUFSIZ];
char expected[BUFSIZ];
char actual[BUFSIZ];

int get_count(sqlite3 *db, char *query) {
    int rc;
    char column_name[BUFSIZ];
    sqlite3_stmt *stmt = NULL;

    if ((rc = sqlite3_prepare_v2(db, query, BUFSIZ, &stmt, 0)) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
    } else while((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch(rc) {
            case SQLITE_BUSY:
                fprintf(stderr, "Database is busy\n");
                sleep(1);
                break;
            case SQLITE_ERROR:
                fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
                return 0;
            case SQLITE_ROW:
                {
                    int n = sqlite3_column_count(stmt);
                    int i;
                    for (i = 0; i < n; i++) {
                        memset(column_name, '\0', BUFSIZ);
                        strcpy(column_name, sqlite3_column_name(stmt, i));

                        if (strcmp(column_name, "count(*)") == 0) {
                            return sqlite3_column_int64(stmt, i);
                        }
                    }
                }
        }
    }

    return 0;
}

void test_json_free_space_array() {
    json_object *jarray = NULL;

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    // Perfect case
    sprintf(test_case, "%s: Should return free space array", __func__);
    sprintf(expected, "[ %d, %d ]", 0, 100);
    jarray = json_free_space_array(0, 100);
    sprintf(actual, "%s", json_object_to_json_string(jarray));

    if (jarray == NULL || strcmp(expected, actual) != 0) {
        test_fail(test_case, expected, actual);
    } else {
        test_pass(test_case);
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    // Start is greater than Final
    sprintf(test_case, "%s: Should be NULL when start is greater than final", __func__);
    jarray = json_free_space_array(100, 0);

    if (jarray != NULL) {
        test_fail(test_case, "null", (char *)json_object_to_json_string(jarray));
    } else {
        test_pass(test_case);
    }

    return;
}

void test_initialize_mapstore() {
    sqlite3 *db = NULL; // Database
    char query[BUFSIZ];

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    sprintf(test_case, "%s: Should successfully initialize context", __func__);
    mapstore_ctx ctx;
    mapstore_opts opts;

    opts.allocation_size = 10737418240; // 10GB
    opts.map_size = 2147483648;         // 2GB
    opts.path = folder;

    if (initialize_mapstore(&ctx, opts) != 0) {
        printf("Error initializing mapstore\n");
        return;
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set allocation size", __func__);
    assert_equal_str(test_case, expected, actual);

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set map_size", __func__);
    assert_equal_int64(test_case, opts.map_size, ctx.map_size);

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set total_mapstores", __func__);
    int expected_total_mapstores = 5;
    assert_equal_int64(test_case, expected_total_mapstores, ctx.total_mapstores);

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set mapstore_path", __func__);
    sprintf(actual, "%s", ctx.mapstore_path);
    sprintf(expected, "%s%cshards%c", folder, separator(), separator());
    assert_equal_str(test_case, expected, actual);

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set database_path", __func__);
    sprintf(actual, "%s", ctx.database_path);
    sprintf(expected, "%s%cshards.sqlite", folder, separator());
    assert_equal_str(test_case, expected, actual);

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should create store directory", __func__);
    struct stat sb;
    if (stat(ctx.mapstore_path, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        test_pass(test_case);
    } else {
        test_fail(test_case, NULL, NULL);
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should prepare 3 tables", __func__);

    /* Open Database */
    if (sqlite3_open_v2(ctx.database_path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        test_fail(test_case, NULL, NULL);
    }

    int count = 0;
    int expected_count = 3;
    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='data_locations';");
    count += get_count(db, query);

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='map_stores';");
    count += get_count(db, query);

    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='mapstore_layout';");
    count += get_count(db, query);

    assert_equal_int64(test_case, expected_count, count);

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should allocate mapstores", __func__);
    test_fail(test_case, NULL, NULL);

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should insert mapstores into database", __func__);
    test_fail(test_case, NULL, NULL);
}

void test_store_data() {
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    sprintf(test_case, "%s: Should successfully store data", __func__);
    test_fail(test_case, NULL, NULL);
}

void test_retrieve_data() {
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    sprintf(test_case, "%s: Should successfully retrieve data", __func__);
    test_fail(test_case, NULL, NULL);
}

void test_delete_data() {
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    sprintf(test_case, "%s: Should successfully delete data", __func__);
    test_fail(test_case, NULL, NULL);
}

void test_get_data_info() {
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    sprintf(test_case, "%s: Should successfully retrieve data meta", __func__);
    test_fail(test_case, NULL, NULL);
}

void test_get_get_store_info() {
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    sprintf(test_case, "%s: Should successfully retrieve store meta", __func__);
    test_fail(test_case, NULL, NULL);
}

int main(void)
{
    // Make sure we have a tmp folder
    if ((folder = getenv("TMPDIR")) == 0) {
        printf("You need to set $TMPDIR before running. (e.g. export TMPDIR=/tmp/)\n");
        exit(1);
    }

    if (folder[strlen(folder)-1] == separator()) {
        folder[strlen(folder)-1] = '\0';
    }

    printf("Test Suite: API\n");
    test_initialize_mapstore();
    // test_store_data();
    // test_retrieve_data();
    // test_delete_data();
    // test_get_data_info();
    // test_get_get_store_info();
    printf("\n");

    printf("Test Suite: Database Utils\n");
    printf("\n");

    printf("Test Suite: Utils\n");
    test_json_free_space_array();
    printf("\n");

    // End Tests
    return test_results();
}
