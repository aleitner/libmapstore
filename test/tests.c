#include "tests.h"
#include "leitner_test.h"

char *folder = NULL;

int main(void)
{
    // Make sure we have a tmp folder
    if ((folder = getenv("TMPDIR")) == 0) {
        printf("You need to set $TMPDIR before running. (e.g. export TMPDIR=/tmp/)\n");
        exit(1);
    }

    printf("Test Suite: API\n");
    printf("\n");

    // End Tests
    return test_results();
}
