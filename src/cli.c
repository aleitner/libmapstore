#include <ctype.h>
#include <getopt.h>
#include "mapstore.h"

static inline void noop() {};

#define HELP_TEXT "usage: mapstore [<options>] <command> [<args>]\n\n"         \
    "These are common mapstore commands for various situations:\n\n"           \
    "  store <data-path>         store file\n"                                 \
    "  retrieve <hash>           retrieve data from map store\n"               \
    "  delete <hash>             delete data from map store\n"                 \
    "  get-data-info <hash>      retrieve data info from map store\n"          \
    "  get-store-info            retrieve store info from map store\n"         \
    "  help [cmd]                display help for [cmd]\n\n"                   \
    "options:\n"                                                               \
    "  -h, --help                output usage information\n"                   \
    "  -v, --version             output the version number\n"                  \
    "  -c, --config              config for mapstore context\n"                \


#define CLI_VERSION "0.0.1"

int main (int argc, char **argv)
{
    int status = 0;
    int c;
    int log_level = 0;
    int index = 0;
    int ret = 0; // Variable for checking function return codes
    char *config_path = NULL;

    static struct option cmd_options[] = {
        {"version", no_argument,  0, 'v'},
        {"log", required_argument,  0, 'l'},
        {"config", required_argument,  0, 'c'},
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
            case 'c':
                config_path = optarg;
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

    mapstore_ctx ctx;
    ret = initialize_ctx(&ctx, config_path);

    /**
     * Store File
     */
    if (strcmp(command, "store") == 0) {
        printf("Storing data\n\n");
        FILE *data_file = NULL;

        char *data_path = argv[command_index + 1];

        if (data_path) {
            data_file = fopen(data_path, "r");;
        } else {
            data_file = stdin;
        }

        if (!data_file) {
            if (data_path) {
                printf("Could not access data: %s\n", data_path);
            } else {
                printf("Could not access data from stdout\n");
            }

            status = 1;
            goto end_store;
        }

        uint8_t *data_hash = NULL;
        ret = get_file_hash(fileno(data_file), &data_hash);
        if (ret != 0) {
            goto end_store;
        }

        printf("Data hash: %s\n", data_hash);
        store_data(&ctx, fileno(data_file), data_hash);

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

    if (strcmp(command, "get-data-info") == 0) {
        printf("Getting data info\n\n");
        return 0;
    }

    if (strcmp(command, "get-store-info") == 0) {
        printf("Getting store info\n\n");
        return 0;
    }

    return status;
}
