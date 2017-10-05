#include "mapstore.h"
#include "utils.h"

MAPSTORE_API int store_data(int fd, uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API int retrieve_data(uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API int delete_data(uint8_t *hash) {
    printf("I'm here!");

    return 0;
}

MAPSTORE_API json_object *get_data_info(uint8_t *hash) {
    printf("I'm here!");
}

MAPSTORE_API json_object *get_store_info() {
    printf("I'm here!");;
}
