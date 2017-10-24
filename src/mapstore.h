
/**
 * @file mapstore.h
 * @brief Memory mapped file storage.
 *
 * Implements functionality to store data across memory mapped files.
 */

#ifndef MAPSTORE_H
#define MAPSTORE_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(MAPSTOREDLL)
  #if defined(DLL_EXPORT)
    #define MAPSTORE_API __declspec(dllexport)
  #else
    #define MAPSTORE __declspec(dllimport)
  #endif
#else
  #define MAPSTORE_API
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <json-c/json.h>
#include <sqlite3.h>
#include <inttypes.h>

#include "utils.h"
#include "database_utils.h"

typedef struct  {
  uint64_t allocation_size;
  uint64_t map_size;
  uint64_t total_mapstores;
  char *mapstore_path;
  char *database_path;
} mapstore_ctx;

typedef struct  {
  uint64_t allocation_size;
  uint64_t map_size;
  char *path;
} mapstore_opts;

typedef struct  {
  uint8_t *hash;
  char *data_locations_array;
} data_info;

typedef struct  {
  uint64_t free_space;
  uint64_t used_space;
  uint64_t allocation_size;
  uint64_t map_size;
} store_info;

MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, uint8_t *hash);
MAPSTORE_API int retrieve_data(mapstore_ctx *ctx, uint8_t *hash);
MAPSTORE_API int delete_data(mapstore_ctx *ctx, uint8_t *hash);
MAPSTORE_API data_info *get_data_info(mapstore_ctx *ctx, uint8_t *hash);
MAPSTORE_API store_info *get_store_info(mapstore_ctx *ctx);
MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts);

static int map_files(mapstore_ctx *ctx);
static int get_map_plan(mapstore_ctx *ctx, uint64_t data_size, json_object *map_coordinates);
static uint64_t store_positions(uint64_t store_id, json_object *free_locations_arr, uint64_t data_size, json_object *map_coordinates);
#ifdef __cplusplus
}
#endif

#endif /* MAPSTORE_H */
