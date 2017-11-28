#include "cli_helper.h"
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
    "  -p, --path <path>         Path toe mapstore\n"                          \
    "  -h, --help                output usage information\n"                   \
    "  -v, --version             output the version number\n"                  \

#define CLI_VERSION "0.0.1"

int main (int argc, char **argv)
{
    int status = 0;
    int c;
    int log_level = 0;
    int index = 0;
    int ret = 0; // Variable for checking function return codes
    char *mapstore_path = NULL;
    uint64_t allocation_size = 0;
    uint64_t map_size = 0;

    static struct option cmd_options[] = {
        {"version", no_argument,  0, 'v'},
        {"log", required_argument,  0, 'l'},
        {"alloc", required_argument,  0, 'a'},
        {"map", required_argument,  0, 'm'},
        {"path", required_argument,  0, 'p'},
        {"debug", no_argument,  0, 'd'},
        {"help", no_argument,  0, 'h'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while ((c = getopt_long_only(argc, argv, "hdl:p:vV:",
                                 cmd_options, &index)) != -1) {
        switch (c) {
            case 'l':
                log_level = atoi(optarg);
                break;
            case 'm':
                map_size = strtoull(optarg, NULL, 10);
                break;
            case 'a':
                allocation_size = strtoull(optarg, NULL, 10);
                break;
            case 'd':
                log_level = 4;
                break;
            case 'p':
                mapstore_path = optarg;
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
            default:
                printf("%c is not a recognized option\n\n", c);
                printf(HELP_TEXT);
                exit(1);
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
    mapstore_opts opts;

    opts.allocation_size = (allocation_size > 0) ? allocation_size : 10737418240; // 10GB
    opts.map_size = (map_size > 0) ? map_size : 2147483648;         // 2GB
    opts.path = (mapstore_path != NULL) ? strdup(mapstore_path) : NULL;

    if (initialize_mapstore(&ctx, opts) != 0) {
        printf("Error initializing mapstore\n");
        return 1;
    }
    printf("Initialized Map_store");

    /**
     * Store File
     */
    if (strcmp(command, "store") == 0) {
        printf("Storing data...\n\n");
        FILE *data_file = NULL;
        char *data_hash = NULL;
        char *data_path = argv[command_index + 1];

        if (data_path) {
            data_file = fopen(data_path, "r");;
        } else {
            data_file = stdin;
        }

        if (!data_file) {
            if (data_path) {
                printf("Failed to access data: %s\n", data_path);
            } else {
                printf("Failed to access data from stdout\n");
            }

            status = 1;
            goto end_store;
        }

        if ((ret = get_file_hash(fileno(data_file), &data_hash)) != 0) {
            printf("Failed to get data hash: %s\n", data_path);
            status = 1;
            goto end_store;
        }

        if ((ret = store_data(&ctx, fileno(data_file), data_hash)) != 0) {
            printf("Failed to store data: %s\n", data_hash);
            status = 1;
            goto end_store;
        }

        printf("Successfully stored data: %s\n", data_hash);

        /* Clean up store command */
end_store:
        if (data_file) {
            fclose(data_file);
        }

        if (data_hash) {
            free(data_hash);
        }

        return status;
    }

    if (strcmp(command, "retrieve") == 0) {
        printf("Retrieving data\n\n");
        char *data_hash = argv[command_index + 1];
        char *retrieval_path = argv[command_index + 2];
        FILE *retrieval_file = NULL;

        if (data_hash == NULL) {
            printf("Missing data hash\n");
            printf(HELP_TEXT);
            status = 1;
            goto end_retrieve;
        }

        if (retrieval_path) {
            retrieval_file = fopen(retrieval_path, "w+");
            if (!retrieval_file) {
                printf("Invalid path: %s\n", retrieval_path);
                status = 1;
                goto end_retrieve;
            }
        } else {
            retrieval_file = stdout;
        }

        if ((ret = retrieve_data(&ctx, fileno(retrieval_file), data_hash)) != 0) {
            printf("Failed to retrieve data: %s\n", data_hash);
            status = 1;
            goto end_retrieve;
        }

        printf("Successfully retrieved data: %s\n", data_hash);

end_retrieve:
        if (retrieval_file) {
            fclose(retrieval_file);
        }

        return status;
    }

    if (strcmp(command, "delete") == 0) {
        char *data_hash = argv[command_index + 1];

        if (data_hash == NULL) {
            printf("Missing data hash\n");
            printf(HELP_TEXT);
            status = 1;
            goto end_delete;
        }

        if ((ret = delete_data(&ctx, data_hash)) != 0) {
            printf("Failed to delete data: %s\n", data_hash);
            status = 1;
            goto end_delete;
        }

        printf("Successfully deleted data: %s\n", data_hash);

end_delete:
        return status;
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
