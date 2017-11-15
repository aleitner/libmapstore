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
  bool uploaded;
} data_locations_row;

typedef struct  {
  uint64_t id;
  json_object *free_locations;
  uint64_t free_space;
  uint64_t size;
} mapstore_row;

typedef struct  {
  int id;
  uint64_t allocation_size;
  uint64_t map_size;
} mapstore_layout_row;

int prepare_tables(char *database_path);
int get_latest_layout_row(sqlite3 *db, mapstore_layout_row *row);
int get_store_rows(sqlite3 *db, char *where, mapstore_row *row);
int get_data_locations_row(sqlite3 *db, char *hash, data_locations_row *row);
int sum_column_for_table(sqlite3 *db, char *column, char *table, uint64_t *sum);
int update_map_store(sqlite3 *db, char *where, char *set);
int insert_to(sqlite3 *db, char *table, char *set);
int hash_exists_in_mapstore(sqlite3 *db, char *hash);
int mark_as_uploaded(sqlite3 *db, char *hash);
int get_pos_from_data_locations(sqlite3 *db, char *hash, json_object **positions);
int delete_by_hash_from_data_locations(sqlite3 *db, char *hash);
int delete_by_id_from_map_stores(sqlite3 *db, uint64_t id);

#endif /* MAPSTORE_DATABASE_UTILS_H */
