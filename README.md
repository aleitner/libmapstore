libmapstore
=============

Store data in file structure

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

#### Initialize the Mapstore CTX at application start

```C
int initialize_mapstore(mapstore_ctx *ctx, mapstore_opts opts);
```

Example:
```C
  mapstore_ctx ctx;
  mapstore_opts opts;

  opts.allocation_size = 10737418240; // 10GB
  opts.map_size = 2147483648;         // 2GB
  opts.path = "~/.store";

  if (initialize_mapstore(&ctx, opts) != 0) {
      printf("Error initializing mapstore\n");
      return 1;
  }
```

#### Store Data

```C
int store_data(mapstore_ctx *ctx, int fd, char *hash);
```

Example:
```C
  mapstore_ctx ctx;
  char *data_hash = "A1B2C3D4E5F6";
  File *data_file = stdin;

  if (initialize_mapstore(&ctx, NULL) != 0) {
      printf("Error initializing mapstore\n");
      return 1;
  }

  if (store_data(&ctx, fileno(data_file), data_hash) != 0) {
      printf("Failed to store data: %s\n", data_hash);
      return 1;
  }
```

#### Retrieve Data

```C
int retrieve_data(mapstore_ctx *ctx, int fd, char *hash);
```

Example:
```C
  mapstore_ctx ctx;
  char *data_hash = "A1B2C3D4E5F6";
  File *retrieval_file = stdout;

  if (initialize_mapstore(&ctx, NULL) != 0) {
      printf("Error initializing mapstore\n");
      return 1;
  }

  if (retrieve_data(&ctx, fileno(retrieval_file), data_hash) != 0) {
      printf("Failed to retrieve data: %s\n", data_hash);
      return 1;
  }
```

#### Delete Data

```C
int delete_data(mapstore_ctx *ctx, char *hash);
```

Example:
```C
  mapstore_ctx ctx;
  char *data_hash = "A1B2C3D4E5F6";

  if (initialize_mapstore(&ctx, NULL) != 0) {
      printf("Error initializing mapstore\n");
      return 1;
  }

  if (delete_data(&ctx, data_hash) != 0) {
      printf("Failed to delete data: %s\n", data_hash);
      return 1;
  }
```

#### Resize Store and/or compact store data
```C
int restructure(mapstore_ctx *ctx, uint64_t map_size, uint64_t alloc_size);
```

Example:
```C
  mapstore_ctx ctx;
  uint64_t map_size = 1000;
  uint64_t allocation_size = 100;

  if (initialize_mapstore(&ctx, NULL) != 0) {
      printf("Error initializing mapstore\n");
      return 1;
  }

  if (restructure(&ctx, map_size, allocation_size) != 0) {
      printf("Failed to restructure\n");
      return 1;
  }
```

#### Get Map Store Metadata

```C
int get_store_info(mapstore_ctx *ctx, store_info *info);
```

Example:
```C
  mapstore_ctx ctx;

  if (initialize_mapstore(&ctx, NULL) != 0) {
      printf("Error initializing mapstore\n");
      return 1;
  }

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
      return 1;
  }


```

#### Get Stored Data's Metadata
```C
int get_data_info(mapstore_ctx *ctx, char *hash, data_info *info);
```

Example:
```C
  mapstore_ctx ctx;

  if (initialize_mapstore(&ctx, NULL) != 0) {
      printf("Error initializing mapstore\n");
      return 1;
  }

  char *data_hash = "A1B2C3D4E5F6";
  data_info info;

  if (get_data_info(&ctx, data_hash, &info) == 0) {
      printf("{ \"hash\": \"%s\", \"size\": %"PRIu64" }\n", info.hash, info.size);
  } else {
      printf("Hash %s does not exist in store.\n", data_hash);
      return 1;
  }
```

### STRUCTS

```C
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
  uint64_t free_space;
  uint64_t used_space;
  uint64_t allocation_size;
  uint64_t map_size;
} store_info;

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
```

## Architecture

TODO: Add graphical example of architecture

### SQLite Database

#### Data location table:

```
--------------------------------------------------------------------
| name | id  | data_hash | data_size | data_positions   | uploaded |
--------------------------------------------------------------------
| type | int | bytes(20) | int64     | Stringified JSON | boolean  |
--------------------------------------------------------------------
```

`data_positions` example:
`{ shard_piece_index: "file_table_id": [ [file_pos, start_pos, end_pos], ... ] }`
```JSON
{ "1": [[0, 10,51], [42, 68, 96]] }
```

#### File table:

```
------------------------------------------------------
| name | id  | free_locations   | free_space | size  |
------------------------------------------------------
| type | int | Stringified JSON | int64      | int64 |
------------------------------------------------------
```

free_locations example:
`[ [start_pos, end_pos], ... ]`
```JSON
[ [0,9], [45,56], [51,51] ]
```
