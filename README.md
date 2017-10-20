libmapstore
=============

Store data in memory mapped files

## Build

```bash
./autogen.sh
./configure
make
```

To run tests:
```bash
./test/tests
```

To run command line utility:
```bash
./src/mapstore --help
```

And to install locally:
```
sudo make install
```

### Debian / Ubuntu (16.04) Dependencies:

Development tools:
```bash
apt-get install build-essential libtool autotools-dev automake
```

Dependencies:
```bash
apt-get install libjson-c-dev libsqlite3-dev libuv1-dev
```

### OS X Dependencies (w/ homebrew):

Development tools:
```bash
brew install libtool automake pkgconfig
```

Dependencies:
```bash
brew install json-c sqlite libuv
```

## API

### FUNCTIONS

```
int store_data(mapstore_ctx *ctx, int fd, uint8_t *hash);
int retrieve_data(mapstore_ctx *ctx, uint8_t *hash);
int delete_data(mapstore_ctx *ctx, uint8_t *hash);
data_info *get_data_info(mapstore_ctx *ctx, uint8_t *hash);
store_info *get_store_info(mapstore_ctx *ctx);
int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts);
```

### STRUCTS

```
typedef struct  {
  uint64_t allocation_size;
  uint64_t map_size;
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
```

## Architecture

TODO: Add graphical example of architecture

### SQLite Database

#### Data location table:

```
-----------------------------------------------------------------
| name | id  | data_hash | data_size | data_positions   | date  |
-----------------------------------------------------------------
| type | int | bytes(20) | int64     | Stringified JSON | int64 |
-----------------------------------------------------------------
```

`data_positions` example:
```
{ shard_piece_index: [file_table_id, start_pos, end_pos], ... }
{ 0: [0,0,9], 1: [1,10,51] }
```
- 1st piece of shard is in file with id `0` from positions `0 to 9`
- 2nd piece of shard is in file with id `1` from positions `10 to 51`

#### File table:
```
----------------------------------------------
| name | id  | free_locations   | free_space |
----------------------------------------------
| type | int | Stringified JSON | int64      |
----------------------------------------------
```

free_locations example:
```
{ [start_pos, end_pos], ... }
{ [0,9], [45,56], [51,51] }
```
