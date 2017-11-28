#include "tests.h"

char *folder = NULL;
FILE *data = NULL;
char *data_hash = NULL;
FILE *data2 = NULL;
char *data_hash2 = NULL;
char test_case[BUFSIZ];
char expected[BUFSIZ];
char actual[BUFSIZ];

int get_file_hash(int fd, char **hash) {
    ssize_t read_len = 0;
    uint8_t read_data[BUFSIZ];
    uint8_t prehash_sha256[SHA256_DIGEST_SIZE];
    uint8_t hash_as_hex[RIPEMD160_DIGEST_SIZE];

    struct sha256_ctx sha256ctx;
    sha256_init(&sha256ctx);

    struct ripemd160_ctx ripemd160ctx;
    ripemd160_init(&ripemd160ctx);

    lseek(fd, 0, SEEK_SET);

    do {
        read_len = read(fd, read_data, BUFSIZ);

        if (read_len == -1) {
            printf("Error reading file for hashing\n");
            return 1;
        }

        sha256_update(&sha256ctx, read_len, read_data);

        memset(read_data, '\0', BUFSIZ);
    } while (read_len > 0);

    memset(prehash_sha256, '\0', SHA256_DIGEST_SIZE);
    sha256_digest(&sha256ctx, SHA256_DIGEST_SIZE, prehash_sha256);

    ripemd160_update(&ripemd160ctx, SHA256_DIGEST_SIZE, prehash_sha256);
    memset(hash_as_hex, '\0', RIPEMD160_DIGEST_SIZE);
    ripemd160_digest(&ripemd160ctx, RIPEMD160_DIGEST_SIZE, hash_as_hex);

    size_t encode_len = BASE16_ENCODE_LENGTH(RIPEMD160_DIGEST_SIZE);
    *hash = calloc(encode_len + 1, sizeof(uint8_t));
    if (!*hash) {
        printf("Error converting hash data to string\n");
        return 1;
    }

    base16_encode_update((uint8_t *)*hash, RIPEMD160_DIGEST_SIZE, hash_as_hex);

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

    opts.allocation_size = 25;
    opts.map_size = 14;
    opts.path = folder;

    if (initialize_mapstore(&ctx, opts) != 0) {
        printf("Error initializing mapstore\n");
        return;
    }

    /* Open Database */
    if (sqlite3_open_v2(ctx.database_path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        test_fail(test_case, NULL, NULL);
    }

    /* */
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set allocation size", __func__);
    assert_equal_str(test_case, expected, actual);

    /* */
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set map_size", __func__);
    assert_equal_int64(test_case, opts.map_size, ctx.map_size);

    /* */
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set total_mapstores", __func__);
    int expected_total_mapstores = (opts.allocation_size / opts.map_size) + ((opts.allocation_size % opts.map_size > 0) ? 1 : 0);
    assert_equal_int64(test_case, expected_total_mapstores, ctx.total_mapstores);

    /* */
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set mapstore_path", __func__);
    sprintf(actual, "%s", ctx.mapstore_path);
    sprintf(expected, "%s%cshards%c", folder, separator(), separator());
    assert_equal_str(test_case, expected, actual);

    /* */
    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set database_path", __func__);
    sprintf(actual, "%s", ctx.database_path);
    sprintf(expected, "%s%cshards.sqlite", folder, separator());
    assert_equal_str(test_case, expected, actual);

    /* */
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should create store directory", __func__);
    struct stat sb;
    if (stat(ctx.mapstore_path, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        test_pass(test_case);
    } else {
        test_fail(test_case, NULL, NULL);
    }

    /* */
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should prepare 3 tables", __func__);

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

    /* */
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should allocate mapstores", __func__);
    char store_path[BUFSIZ];
    struct stat st;
    count = 0;
    for (int i = 1; i <= ctx.total_mapstores; i++) {
        memset(store_path, '\0', BUFSIZ);
        sprintf(store_path, "%s%d.map", ctx.mapstore_path, i);
        if (stat(store_path, &st) == 0) {
            if ((i == ctx.total_mapstores && st.st_size == ctx.allocation_size % ctx.map_size) ||
                st.st_size == ctx.map_size) {
                count +=1;
            }
        }
    }
    assert_equal_int64(test_case, ctx.total_mapstores, count);

    /* Test if data was inserted to map_stores */
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should insert mapstore meta into database", __func__);
    count = 0;
    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT count(*) FROM 'map_stores';");
    count += get_count(db, query);
    assert_equal_int64(test_case, ctx.total_mapstores, count);

    /* Test if data was inserted to mapstore_layout */
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should insert mapstore_layout meta into database", __func__);
    mapstore_layout_row row;
    get_latest_layout_row(db, &row);
    assert_equal_int64(test_case, ctx.map_size, row.map_size);
    assert_equal_int64(test_case, ctx.allocation_size, row.allocation_size);


    /* Delete everything */
    for (int i = 1; i <= ctx.total_mapstores; i++) {
        memset(store_path, '\0', BUFSIZ);
        sprintf(store_path, "%s%d.map", ctx.mapstore_path, i);
        remove(store_path);
    }
    remove(ctx.database_path);
    if (db) {
        sqlite3_close(db);
    }
}

void test_store_data() {
    sqlite3 *db = NULL; // Database
    char store_path[BUFSIZ];
    char query[BUFSIZ];

    memset(expected, '\0', BUFSIZ);
    memset(actual, '\0', BUFSIZ);
    memset(test_case, '\0', BUFSIZ);

    sprintf(test_case, "%s: Should successfully initialize context", __func__);
    mapstore_ctx ctx;
    mapstore_opts opts;

    opts.allocation_size = 512; // 10GB
    opts.map_size = 128;         // 2GB
    opts.path = folder;

    if (initialize_mapstore(&ctx, opts) != 0) {
        printf("Error initializing mapstore\n");
        return;
    }

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should successfully store data", __func__);
    if ((store_data(&ctx, fileno(data), data_hash)) != 0) {
        test_fail(test_case, NULL, NULL);
    }

    /* Open Database */
    if (sqlite3_open_v2(ctx.database_path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        test_fail(test_case, NULL, NULL);
    }

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should insert data meta into database", __func__);
    int count = 0;
    memset(query, '\0', BUFSIZ);
    sprintf(query, "SELECT count(*) FROM 'data_locations';");
    count += get_count(db, query);
    assert_equal_int64(test_case, 1, count);

    data_locations_row row;
    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should insert hash into database", __func__);
    get_data_locations_row(db, data_hash, &row);
    assert_equal_str(test_case, data_hash, row.hash);

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should mark data uploaded as true", __func__);
    assert_equal_int64(test_case, true, row.uploaded);

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should mark data size as 256", __func__);
    assert_equal_int64(test_case, 256, row.size);

    memset(test_case, '\0', BUFSIZ);
    memset(expected, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should set positions in map store", __func__);
    sprintf(expected, "{ \"1\": [ [ 0, 0, 127 ] ], \"2\": [ [ 128, 0, 127 ] ] }");
    assert_equal_str(test_case, expected, (char *)json_object_to_json_string(row.positions));

    char where[BUFSIZ];
    mapstore_row store_row;
    for (int i = 1; i <= ctx.total_mapstores; i++) {
        memset(where, '\0', BUFSIZ);
        sprintf(where, "WHERE Id='%d'", i);
        get_store_rows(db, where, &store_row);

        memset(expected, '\0', BUFSIZ);
        if (i == 1 || i == 2) {
            memset(test_case, '\0', BUFSIZ);
            sprintf(test_case, "%s: Should update free_space for map %d", __func__, i);
            assert_equal_int64(test_case, 0, store_row.free_space);

            memset(test_case, '\0', BUFSIZ);
            sprintf(test_case, "%s: Should update free_locations for map %d", __func__, i);
            sprintf(expected, "[ ]");
            assert_equal_str(test_case, expected, (char *)json_object_to_json_string(store_row.free_locations));
        } else {
            memset(test_case, '\0', BUFSIZ);
            sprintf(test_case, "%s: Should update free_space for map %d", __func__, i);
            assert_equal_int64(test_case, 128, store_row.free_space);

            memset(test_case, '\0', BUFSIZ);
            sprintf(test_case, "%s: Should update free_locations for map %d", __func__, i);
            sprintf(expected, "[ [ 0, 127 ] ]");
            assert_equal_str(test_case, expected, (char *)json_object_to_json_string(store_row.free_locations));
        }
    }

    memset(test_case, '\0', BUFSIZ);
    sprintf(test_case, "%s: Should fail to store data with same hash", __func__);
    if ((store_data(&ctx, fileno(data), data_hash)) != 0) {
        test_pass(test_case);
    } else {
        test_fail(test_case, NULL, NULL);
    }

    for (int i = 1; i <= ctx.total_mapstores; i++) {
        memset(store_path, '\0', BUFSIZ);
        sprintf(store_path, "%s%d.map", ctx.mapstore_path, i);
        remove(store_path);
    }
    remove(ctx.database_path);
    if (db) {
        sqlite3_close(db);
    }
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

    char file_path[BUFSIZ];
    memset(file_path, '\0', BUFSIZ);
    sprintf(file_path, "%stest.data", folder);
    if (!(data = fopen(file_path, "w+"))) {
        return 1;
    }

    char file_path2[BUFSIZ];
    memset(file_path2, '\0', BUFSIZ);
    sprintf(file_path2, "%stest.data", folder);
    if (!(data2 = fopen(file_path2, "w+"))) {
        return 1;
    }

    time_t t;
    srand((unsigned) time(&t));
    for (int i = 0; i <= 256; i++) {
        fseek(data, i, SEEK_SET);
        fputc("ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 26], data);

        fseek(data2, i, SEEK_SET);
        fputc("ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 26], data2);
    }

    if ((get_file_hash(fileno(data), &data_hash)) != 0) {
        test_fail(test_case, NULL, NULL);
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
    if (data) {
        fclose(data);
    }

    if (data2) {
        fclose(data2);
    }

    remove(file_path);
    remove(file_path2);

    if (data_hash) {
        free(data_hash);
    }

    if (data_hash2) {
        free(data_hash2);
    }

    return test_results();
}
