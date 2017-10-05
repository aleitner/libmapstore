#include <ctype.h>
#include <getopt.h>
#include "mapstore.h"

static inline void noop() {};

#define HELP_TEXT "usage: mapstore [<options>] <command> [<args>]\n\n"         \
    "These are common mapstore commands for various situations:\n\n"           \
    "  store <data-path>       store file\n"                                 \
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

    /**
     * Store File
     */
    if (strcmp(command, "store") == 0) {
        printf("Storing data\n\n");
        FILE *data_file = NULL;

        char *data_path = argv[command_index + 1];

        if (!data_path) {
            printf("Missing first argument: <data-path>\n");
            status = 1;
            goto end_store;
        }

        data_file = fopen(data_path, "r");

        if (!data_file) {
            printf("Could not access data: %s\n", data_path);
            status = 1;
            goto end_store;
        }

        uint8_t *data_hash = "abc123";

        status = store_data(fileno(data_file), data_hash);

        /* Clean up store command */
end_store:
        if (data_file) {
            fclose(data_file);
        }

        return status;
    }

    if (strcmp(command, "retrieve") == 0) {
        printf("Retrieving data\n\n");
        return 0;
    }

    if (strcmp(command, "delete") == 0) {
        printf("Deleting data\n\n");
        return 0;
    }

    return status;
}
