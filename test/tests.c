#include "tests.h"

char *folder = NULL;

void test_json_free_space_array() {
    json_object *jarray = NULL;
    char expected[BUFSIZ];
    char actual[BUFSIZ];
    char test_case[BUFSIZ];

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

int main(void)
{
    // Make sure we have a tmp folder
    if ((folder = getenv("TMPDIR")) == 0) {
        printf("You need to set $TMPDIR before running. (e.g. export TMPDIR=/tmp/)\n");
        exit(1);
    }

    printf("Test Suite: API\n");
    printf("\n");

    printf("Test Suite: Database Utils\n");
    printf("\n");

    printf("Test Suite: Utils\n");
    test_json_free_space_array();
    printf("\n");

    // End Tests
    return test_results();
}
