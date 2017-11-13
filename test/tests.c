#include "tests.h"

char *folder = NULL;
char test_case[BUFSIZ];
char expected[BUFSIZ];
char actual[BUFSIZ];

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

    sprintf(test_case, "%s: Should set allocation size", __func__);
    sprintf(actual, "%"PRIu64, ctx.allocation_size);
    sprintf(expected, "%"PRIu64, opts.allocation_size);
    if (ctx.allocation_size == opts.allocation_size) {
        test_pass(test_case);
    } else {
        test_fail(test_case, expected, actual);
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set map_size", __func__);
    sprintf(actual, "%"PRIu64, ctx.map_size);
    sprintf(expected, "%"PRIu64, opts.map_size);
    if (ctx.map_size == opts.map_size) {
        test_pass(test_case);
    } else {
        test_fail(test_case, expected, actual);
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set total_mapstores", __func__);
    sprintf(actual, "%"PRIu64, ctx.total_mapstores);
    sprintf(expected, "5");
    if (ctx.total_mapstores == 5) {
        test_pass(test_case);
    } else {
        test_fail(test_case, expected, actual);
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set mapstore_path", __func__);
    sprintf(actual, "%s", ctx.mapstore_path);
    sprintf(expected, "%s%cshards%c", folder, separator(), separator());
    if (strcmp(expected, actual) == 0) {
        test_pass(test_case);
    } else {
        test_fail(test_case, expected, actual);
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set database_path", __func__);
    sprintf(actual, "%s", ctx.database_path);
    sprintf(expected, "%s%cshards.sqlite", folder, separator());
    if (strcmp(expected, actual) == 0) {
        test_pass(test_case);
    } else {
        test_fail(test_case, expected, actual);
    }

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should create store directory", __func__);
    test_fail(test_case, NULL, NULL);

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should prepare_tables", __func__);
    test_fail(test_case, NULL, NULL);

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
    test_store_data();
    test_retrieve_data();
    test_delete_data();
    test_get_data_info();
    test_get_get_store_info();
    printf("\n");

    printf("Test Suite: Database Utils\n");
    printf("\n");

    printf("Test Suite: Utils\n");
    test_json_free_space_array();
    printf("\n");

    // End Tests
    return test_results();
}
