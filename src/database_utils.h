/**
 * @file database_utils.h
 * @brief Map Store database utility functions.
 *
 * Database utilities
 */
#ifndef MAPSTORE_DATABASE_UTILS_H
#define MAPSTORE_DATABASE_UTILS_H
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sqlite3.h>

#include "mapstore.h"
#include "utils.h"

typedef struct  {
  int id;
  char *hash;
  uint64_t size;
  json_object *positions;
  uint64_t date;
} data_locations_row;

typedef struct  {
  uint64_t id;
  json_object *free_locations;
  uint64_t free_space;
  uint64_t size;
} map_store_row;

typedef struct  {
  int id;
  uint64_t allocation_size;
  uint64_t map_size;
} map_store_layout_row;

int prepare_tables(char *database_path);
int map_files(mapstore_ctx *ctx);
int get_latest_layout_row(sqlite3 *db, map_store_layout_row *row);
int get_store_row_by_id(sqlite3 *db, uint64_t id, map_store_row *row);

#endif /* MAPSTORE_DATABASE_UTILS_H */
