
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
#include "utils.h"

#include <inttypes.h>

typedef struct  {
  uint64_t allocation_size;
  uint64_t map_size;
} mapstore_ctx;

typedef struct  {
  uint64_t allocation_size;
  uint64_t map_size;
} mapstore_opts;

MAPSTORE_API int store_data(mapstore_ctx *ctx, int fd, uint8_t *hash);
MAPSTORE_API int retrieve_data(mapstore_ctx *ctx, uint8_t *hash);
MAPSTORE_API int delete_data(mapstore_ctx *ctx, uint8_t *hash);
MAPSTORE_API json_object *get_data_info(mapstore_ctx *ctx, uint8_t *hash);
MAPSTORE_API json_object *get_store_info(mapstore_ctx *ctx);
MAPSTORE_API int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts);

#ifdef __cplusplus
}
#endif

#endif /* MAPSTORE_H */
