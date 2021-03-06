/**
 * @file utils.h
 * @brief Map Store utilities.
 *
 * Helper utilities
 */
#ifndef MAPSTORE_UTILS_H
#define MAPSTORE_UTILS_H
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <math.h>

#include <json-c/json.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <time.h>
#include <io.h>

ssize_t pread(int fd, void *buf, size_t count, uint64_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, uint64_t offset);

#else
#include <sys/time.h>
#include <sys/mman.h>
#endif

#include "utils.h"
#include "database_utils.h"

int allocatefile(int fd, uint64_t length);
int unmap_file(uint8_t *map, uint64_t filesize);
int map_file(int fd, uint64_t filesize, uint8_t **map, bool read_only);
int create_directory(char *path);
int create_map_store(char *path, uint64_t size, bool prealloc);
int write_to_store(int data_fd, char *store_dir, json_object *data_locations);
int read_from_store(int output_fd, char *store_dir, json_object *data_locations);
uint64_t get_file_size(int fd);
uint64_t sector_min(uint64_t data_size);
uint64_t prepare_store_positions(uint64_t store_id,
                                 json_object *free_locations_arr,
                                 uint64_t data_position,
                                 uint64_t data_size,
                                 json_object *map_plan);

/* Json Functions */
json_object *json_free_space_array(uint64_t start, uint64_t end);
json_object *json_data_positions_array(uint64_t file_pos, uint64_t start, uint64_t end);
json_object *expand_free_space_list(json_object *old_free_space, uint64_t old_size, uint64_t new_size);
json_object *combine_positions(json_object *locations, uint64_t *freespace);

static inline char separator()
{
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

#endif /* MAPSTORE_UTILS_H */
