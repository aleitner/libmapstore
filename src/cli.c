#include "cli_helper.h"
#include "mapstore.h"

static inline void noop() {};

#define HELP_TEXT "usage: mapstore [<options>] <command> [<args>]\n\n"         \
    "These are common mapstore commands for various situations:\n\n"           \
    "  store <data-path>         store file\n"                                 \
    "  stream                    stream data into store\n"                     \
    "  retrieve <hash>           retrieve data from map store\n"               \
    "  delete <hash>             delete data from map store\n"                 \
    "  get-data-info <hash>      retrieve data info from map store\n"          \
    "  get-store-info            retrieve store info from map store\n"         \
    "  help [cmd]                display help for [cmd]\n\n"                   \
    "options:\n"                                                               \
    "  -p, --path <path>         Path toe mapstore\n"                          \
    "  -h, --help                output usage information\n"                   \
    "  -v, --version             output the version number\n"                  \

#define CLI_VERSION "1.0.0"

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

    /**
     * Store File
     */
    if (strcmp(command, "store") == 0) {
        printf("Storing data...\n\n");
        int flags = 0;
        glob_t results;
        int ret = 0;

        for (int i = command_index + 1; i < argc; i++) {
            flags |= (i > command_index + 1 ? GLOB_APPEND : 0);

            if ((ret = glob(argv[i], flags, 0, &results)) != 0) {
    			fprintf(stderr, "%s: problem with %s (%s), stopping early\n",
    				argv[0], argv[i],
    		        (ret == GLOB_ABORTED ? "filesystem problem" :
    				 ret == GLOB_NOMATCH ? "no match of pattern" :
    				 ret == GLOB_NOSPACE ? "no dynamic memory" :
    				 "unknown problem"));
                goto end_store;
    		}
        }

        for (int i = 0; i < results.gl_pathc; i++) {
            FILE *data_file = NULL;
            char *data_hash = NULL;
            char *data_path = NULL;

    		data_path = (results.gl_pathc == 0) ? NULL : results.gl_pathv[i];
            data_file = fopen(data_path, "r");

            if (!data_file) {
                printf("Failed to access data: %s\n", data_path);
                continue;
            }

            if ((ret = get_file_hash(fileno(data_file), &data_hash)) != 0) {
                printf("Failed to get data hash: %s\n", data_path);
                status = 1;
            } else {
                if ((ret = store_data(&ctx, fileno(data_file), data_hash)) != 0) {
                    printf("Failed to store data: %s\n", data_hash);
                    status = 1;
                }
            }

            if (status == 0) {
                printf("Successfully stored data: %s\n", data_hash);
            }

            if (data_file) {
                fclose(data_file);
            }

            if (data_hash) {
                free(data_hash);
            }
        }

        /* Clean up store command */
end_store:
        globfree(&results);

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

    if (strcmp(command, "stream") == 0) {
        char *data_hash = NULL;

        if (get_file_hash(fileno(stdin), &data_hash) != 0) {
            printf("Failed to get data hash from stdin\n");
            status = 1;
            goto end_stream;
        }

        if (store_data(&ctx, fileno(stdin), data_hash) != 0) {
            printf("Failed to store data: %s\n", data_hash);
            status = 1;
            goto end_stream;
        }

        printf("Successfully stored data: %s\n", data_hash);

end_stream:
        if (data_hash) {
            free(data_hash);
        }

        return status;
    }

    if (strcmp(command, "get-data-info") == 0) {
        char *data_hash = argv[command_index + 1];

        data_info info;
        if ((status = get_data_info(&ctx, data_hash, &info)) == 0) {
            printf("{ \"hash\": \"%s\", \"size\": %"PRIu64" }\n", info.hash, info.size);
        } else {
            printf("Hash %s does not exist in store.\n", data_hash);
        }

        return status;
    }

    if (strcmp(command, "get-store-info") == 0) {
        store_info info;
        if ((status = get_store_info(&ctx, &info)) == 0) {
            printf("{ \"free_space\": %"PRIu64", "    \
                    "\"used_space\": %"PRIu64", "     \
                    "\"allocation_size\": %"PRIu64", "\
                    "\"map_size\": %"PRIu64", "       \
                    "\"data_count\": %"PRIu64", "     \
                    "\"total_stores\": %"PRIu64" "    \
                    "}\n",                            \
                    info.free_space,
                    info.used_space,
                    info.allocation_size,
                    info.map_size,
                    info.data_count,
                    info.total_mapstores);
        } else {
            printf("Failed to get store info.\n");
        }

        return status;
    }

    return status;
}
