
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
#include <uv.h>

#include <inttypes.h>

#ifdef __cplusplus
}
#endif

#endif /* MAPSTORE_H */
