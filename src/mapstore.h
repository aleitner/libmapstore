
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
  char *base_path;
  sqlite3 *db;
  bool prealloc;
} mapstore_ctx;

typedef struct  {
  uint64_t allocation_size;
  uint64_t map_size;
  char *path;
  bool prealloc;
} mapstore_opts;

typedef struct  {
  char *hash;
  uint64_t size;
} data_info;

typedef struct  {
  uint64_t free_space;
  uint64_t used_space;
  uint64_t allocation_size;
  uint64_t map_size;
  uint64_t data_count;
  uint64_t total_mapstores;
} store_info;

MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, char *hash);
MAPSTORE_API int retrieve_data(mapstore_ctx *ctx, int fd, char *hash);
MAPSTORE_API int delete_data(mapstore_ctx *ctx, char *hash);
MAPSTORE_API int get_data_info(mapstore_ctx *ctx, char *hash, data_info *info);
MAPSTORE_API int get_store_info(mapstore_ctx *ctx, store_info *info);
MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts);
MAPSTORE_API int restructure(mapstore_ctx *ctx, uint64_t map_size, uint64_t alloc_size);

int get_map_plan(sqlite3 *db, uint64_t total_stores, uint64_t data_size, json_object *map_coordinates);
int get_updated_free_locations(sqlite3 *db, json_object *positions, json_object **updated_positions);

#ifdef __cplusplus
}
#endif

#endif /* MAPSTORE_H */
