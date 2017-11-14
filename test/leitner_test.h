#ifndef LEITNERTEST_H
#define LEITNERTEST_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <assert.h>
#include <stddef.h>

#define KRED    "\x1B[31m"
#define KGRN    "\x1B[32m"
#define RESET   "\x1B[0m"

int tests_ran = 0;
int tests_passed = 0;
int tests_failed = 0;

int test_results();
void test_fail(char *msg, char *expected, char *actual);
void test_pass(char *msg);
void assert_equal_int64(char *test_case, uint64_t expected, uint64_t actual);
void assert_equal_str(char *test_case, char *expected, char *actual);

void test_fail(char *msg, char *expected, char *actual) {
    printf("\t" KRED "FAIL" RESET " %s\n", msg);

    if (expected && actual) {
        printf("\t\texpected: %s ", expected);
        printf("\tactual:   %s\n", actual);
    }

    tests_failed += 1;
    tests_ran += 1;
}

void test_pass(char *msg) {
    printf("\t" KGRN "PASS" RESET " %s\n", msg);
    tests_passed += 1;
    tests_ran += 1;
}

int test_results() {
    printf(KGRN "\nPASSED: %i" RESET, tests_passed);
    if (tests_failed > 0) {
        printf(KRED " FAILED: %i" RESET, tests_failed);
    }
    printf(" TOTAL: %i\n", (tests_ran));

    if (tests_failed > 0) {
        return 1;
    }

    return 0;
}

void assert_equal_int64(char *test_case, uint64_t expected, uint64_t actual) {
    char *actual_str = NULL;
    char *expected_str = NULL;

    if (expected == actual) {
        test_pass(test_case);
    } else {
        asprintf(&actual_str, "%"PRIu64, actual);
        asprintf(&expected_str, "%"PRIu64, expected);
        test_fail(test_case, expected_str, actual_str);
    }

    free(actual_str);
    free(expected_str);

    return;
}

void assert_equal_str(char *test_case, char *expected, char *actual) {
    if (strcmp(expected, actual) == 0) {
        test_pass(test_case);
    } else {
        test_fail(test_case, expected, actual);
    }

    return;
}

#ifdef __cplusplus
}
#endif

#endif /* LEITNERTEST_H */
