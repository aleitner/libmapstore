#ifndef LEITNERTEST_H
#define LEITNERTEST_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <assert.h>

#define KRED    "\x1B[31m"
#define KGRN    "\x1B[32m"
#define RESET   "\x1B[0m"

int tests_ran = 0;
int tests_passed = 0;
int tests_failed = 0;

int test_results();
void test_fail(char *msg, char *expected, char *actual);
void test_pass(char *msg);

void test_fail(char *msg, char *expected, char *actual) {
    printf("\t" KRED "FAIL" RESET " %s\n", msg);
    printf("\t\texpected array: %s\n", expected);
    printf("\t\tactual array:   %s\n", actual);
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

#ifdef __cplusplus
}
#endif

#endif /* LEITNERTEST_H */
