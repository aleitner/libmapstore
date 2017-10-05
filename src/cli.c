#include <ctype.h>
#include <getopt.h>
#include "mapstore.h"

static inline void noop() {};

#define HELP_TEXT "usage: mapstore [<options>] <command> [<args>]\n\n"         \
    "These are common mapstore commands for various situations:\n\n"           \
    "  store <config-path>       store file\n"                                 \
    "  retrieve <hash>           retrieve data from map store\n"               \
    "  delete <hash>             delete data from map store\n"                 \
    "  help [cmd]                display help for [cmd]\n\n"                   \
    "options:\n"                                                               \
    "  -h, --help                output usage information\n"                   \
    "  -v, --version             output the version number\n"                  \


#define CLI_VERSION "0.0.1"

int main (int argc, char **argv)
{
    int status = 0;
    int c;
    int log_level = 0;
    int index = 0;

    static struct option cmd_options[] = {
        {"version", no_argument,  0, 'v'},
        {"log", required_argument,  0, 'l'},
        {"debug", no_argument,  0, 'd'},
        {"help", no_argument,  0, 'h'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while ((c = getopt_long_only(argc, argv, "hdl:vV:",
                                 cmd_options, &index)) != -1) {
        switch (c) {
            case 'l':
                log_level = atoi(optarg);
                break;
            case 'd':
                log_level = 4;
                break;
            case 'V':
            case 'v':
                printf(CLI_VERSION "\n\n");
                exit(0);
                break;
            case 'h':
                printf(HELP_TEXT);
                exit(0);
                break;
        }
    }

    if (log_level > 4 || log_level < 0) {
        printf("Invalid log level\n");
        return 1;
    }

    int command_index = optind;

    char *command = argv[command_index];
    if (!command) {
        printf(HELP_TEXT);
        return 0;
    }


    if (strcmp(command, "store") == 0) {
        printf("Storing data\n\n");
        return 1;
    }

    if (strcmp(command, "retrieve") == 0) {
        printf("Retrieving data\n\n");
        return 1;
    }

    if (strcmp(command, "delete") == 0) {
        printf("Deleting data\n\n");
        return 1;
    }

    return status;
}
