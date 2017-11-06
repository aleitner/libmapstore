#include "utils.h"

#ifdef _WIN32
ssize_t pread(int fd, void *buf, size_t count, uint64_t offset)
{
    long unsigned int read_bytes = 0;

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(OVERLAPPED));

    overlapped.OffsetHigh = (uint32_t)((offset & 0xFFFFFFFF00000000LL) >> 32);
    overlapped.Offset = (uint32_t)(offset & 0xFFFFFFFFLL);

    HANDLE file = (HANDLE)_get_osfhandle(fd);
    SetLastError(0);
    bool RF = ReadFile(file, buf, count, &read_bytes, &overlapped);

     // For some reason it errors when it hits end of file so we don't want to check that
    if ((RF == 0) && GetLastError() != ERROR_HANDLE_EOF) {
        errno = GetLastError();
        // printf ("Error reading file : %d\n", GetLastError());
        return -1;
    }

    return read_bytes;
}

ssize_t pwrite(int fd, const void *buf, size_t count, uint64_t offset)
{
    long unsigned int written_bytes = 0;

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(OVERLAPPED));

    overlapped.OffsetHigh = (uint32_t)((offset & 0xFFFFFFFF00000000LL) >> 32);
    overlapped.Offset = (uint32_t)(offset & 0xFFFFFFFFLL);

    HANDLE file = (HANDLE)_get_osfhandle(fd);
    SetLastError(0);
    bool RF = WriteFile(file, buf, count, &written_bytes, &overlapped);
    if ((RF == 0)) {
        errno = GetLastError();
        // printf ("Error reading file :%d\n", GetLastError());
        return -1;
    }

    return written_bytes;
}
#endif

int allocatefile(int fd, uint64_t length)
{
#ifdef _WIN32
    HANDLE file = (HANDLE)_get_osfhandle(fd);
    if (file == INVALID_HANDLE_VALUE) {
        return EBADF;
    }

    int status = 0;

    LARGE_INTEGER size;
    size.HighPart = (uint32_t)((length & 0xFFFFFFFF00000000LL) >> 32);
    size.LowPart = (uint32_t)(length & 0xFFFFFFFFLL);

    if (!SetFilePointerEx(file, size, 0, FILE_BEGIN)) {
        status = GetLastError();
        goto win_finished;
    }

    if (!SetEndOfFile(file)) {
        status = GetLastError();
        goto win_finished;
    }

win_finished:

    return status;
#elif HAVE_POSIX_FALLOCATE
    return posix_fallocate(fd, 0, length);
#elif __unix__
    if (fallocate(fd, 0, 0, length)) {
        return errno;
    }
    return 0;
#elif __linux__
    if (fallocate(fd, 0, 0, length)) {
        return errno;
    }
    return 0;
#elif __APPLE__
    fstore_t store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, length, 0};
    // Try to get a continous chunk of disk space
    int ret = fcntl(fd, F_PREALLOCATE, &store);
    if (-1 == ret) {
        // OK, perhaps we are too fragmented, allocate non-continuous
        store.fst_flags = F_ALLOCATEALL;
        ret = fcntl(fd, F_PREALLOCATE, &store);
        if ( -1 == ret) {
            return -1;
        }
    }
    return ftruncate(fd, length);
#else
    return -1;
#endif
}


int unmap_file(uint8_t *map, uint64_t filesize)
{
#ifdef _WIN32
    if (!FlushViewOfFile(map, filesize)) {
        return GetLastError();
    }
    if (!UnmapViewOfFile(map)) {
        return GetLastError();
    }
#else
    if (munmap(map, filesize)) {
        return errno;
    }
#endif
    return 0;
}

int map_file(int fd, uint64_t filesize, uint8_t **map, bool read_only)
{
    int status = 0;
#ifdef _WIN32
    HANDLE fh = (HANDLE)_get_osfhandle(fd);
    if (fh == INVALID_HANDLE_VALUE) {
        return EBADF;
    }

    int prot = read_only ? PAGE_READONLY : PAGE_READWRITE;

    HANDLE mh = CreateFileMapping(fh, NULL, prot, 0, 0, NULL);
    if (!mh) {
        status = GetLastError();
        goto win_finished;
    }

    prot = read_only ? FILE_MAP_READ : FILE_MAP_WRITE;

    *map = MapViewOfFileEx(mh, prot, 0, 0, filesize, NULL);
    if (!*map) {
        status = GetLastError();
        goto win_finished;
    }

win_finished:
    CloseHandle(mh);
#else
    int prot = read_only ? PROT_READ : PROT_READ | PROT_WRITE;
    *map = (uint8_t *)mmap(NULL, filesize, prot, MAP_SHARED, fd, 0);
    if (*map == MAP_FAILED) {
        status = errno;
    }
#endif
    return status;
}

int create_directory(char *path) {
#ifdef _WIN32
    if (CreateDirectory(path, NULL) != 0) {
        return 1
    }

    return 0;
#else
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        if (mkdir(path, 0777) != 0) {
            perror("mkdir");
            return 1;
        }
    }

    return 0;
#endif
}

json_object *json_free_space_array(uint64_t start, uint64_t end) {
    if (end < start) {
        return NULL;
    }
    json_object *jarray = json_object_new_array();
    json_object_array_add(jarray, json_object_new_int64(start));
    json_object_array_add(jarray, json_object_new_int64(end));

    return jarray;
}

json_object *json_data_positions_array(uint64_t file_id, uint64_t start, uint64_t end) {
    json_object *jarray = json_object_new_array();
    json_object_array_add(jarray, json_object_new_int64(file_id));
    json_object_array_add(jarray, json_object_new_int64(start));
    json_object_array_add(jarray, json_object_new_int64(end));
    return jarray;
}

json_object *expand_free_space_list(json_object *old_free_space, uint64_t old_size, uint64_t new_size) {
    json_object *old_jarray = NULL;
    json_object *new_jarray = NULL;

    uint64_t old_first;
    uint64_t old_final;

    json_object *new_free_space = json_object_new_array();
    bool needs_new_coordinates = true; // true if we need to add new coordinates. false if we modify old coordinates
    int i = 0;

    printf("Old Size: %"PRIu64", New Size: %"PRIu64"\n", old_size, new_size);
    printf("Old_space: %s\n",json_object_to_json_string(old_free_space));

    if (old_free_space == NULL) {
        new_jarray = json_free_space_array(0, new_size - 1);
        json_object_array_add(new_free_space, new_jarray);

        return new_free_space;
    }

    if (old_size < new_size) {
        for (i = 0; i < json_object_array_length(old_free_space); i++) {
            old_jarray = json_object_array_get_idx(old_free_space, i);
            old_first = json_object_get_int64(json_object_array_get_idx(old_jarray, 0));
            old_final = json_object_get_int64(json_object_array_get_idx(old_jarray, 1));

            /* are we changing anything? */
            if (old_final == old_size - 1) {
                new_jarray = json_free_space_array(old_first, new_size - 1);
                needs_new_coordinates = false;
                json_object_array_add(new_free_space, new_jarray);
            } else {
                json_object_array_add(new_free_space, old_jarray);
            }
        }

        if (needs_new_coordinates) {
            new_jarray = json_free_space_array(old_size - 1, new_size - 1);
            json_object_array_add(new_free_space, new_jarray);
        }
    } else {
        return old_free_space;
    }

    printf("new_space: %s\n",json_object_to_json_string(new_free_space));

    return new_free_space;
}

int create_map_store(char *path, uint64_t size) {
    int status = 0;
    uint8_t *mmap_store = NULL;         // Memory Mapped map_store
    FILE *fmap_store = fopen(path, "w+");

    printf("Path: %s, size: %"PRIu64"\n", path, size);

    int falloc_status = allocatefile(fileno(fmap_store), size);

    if (falloc_status) {
        status = 1;
        fprintf(stdout, "Could not allocate space for mmap parity " \
                         "shard file: %i", falloc_status);
        goto create_map_store;
    }

    map_file(fileno(fmap_store), size, &mmap_store, false);
    unmap_file(mmap_store, size);

create_map_store:
    if (fmap_store) {
        fclose(fmap_store);
    }

    return status;
}

uint64_t get_file_size(int fd) {
    int ret = 0;
    struct stat st;

    ret = fstat(fd, &st);
    if (ret != 0) {
        printf("Could not get file size\n");
        return 0;
    }

    return st.st_size;
}

uint64_t sector_min(uint64_t data_size) {
    // TODO: Optimize minimum piece separation for data
    return 0;
}

uint64_t prepare_store_positions(uint64_t store_id, json_object *free_locations_arr, uint64_t data_size, json_object *map_plan) {
    uint64_t sector_size = 0;            //
    uint64_t space_to_use = 0;           //
    json_object *location_array = NULL;  // json object containing free location array
    uint64_t first;                      // free location start for array
    uint64_t old_final, new_final;       // free location end for array
    uint64_t total_used = 0;
    uint64_t remaining = data_size;
    char *store_id_str;
    asprintf(&store_id_str, "%"PRIu64, store_id);

    // Get previously added store_positions
    json_object *store_positions = json_object_new_array();

    // Get list of updated freespaces
    json_object *updated_free_positions = json_object_new_array();

    // Get previously added store_positions
    json_object *used_space = json_object_new_array();

    json_object *map_store_obj = json_object_new_object();

    for (uint64_t arr_i = 0; arr_i < json_object_array_length(free_locations_arr); arr_i++) {

        location_array = json_object_array_get_idx(free_locations_arr, arr_i);
        first = json_object_get_int64(json_object_array_get_idx(location_array, 0));
        old_final = json_object_get_int64(json_object_array_get_idx(location_array, 1));
        sector_size = old_final - first + 1;

        // If there isn't enough space in the free_space sector don't use it.
        if (remaining <= 0 || sector_size < sector_min(data_size)) {
            json_object_array_add(updated_free_positions, json_free_space_array(first, old_final));
            continue;
        }

        // Calculate the amount of space to be stored
        space_to_use = (sector_size > remaining) ? remaining : sector_size;
        new_final = (sector_size > remaining) ? first + space_to_use - 1 : old_final;
        total_used += space_to_use;
        remaining -= space_to_use;

        // Create coordinates for data piece to be stored and add to store_positions array
        json_object_array_add(store_positions, json_free_space_array(first, new_final));

        // If modifying the free space
        if (old_final != new_final) {
            json_object_array_add(updated_free_positions, json_free_space_array(new_final + 1, old_final));
        }
    }

    // Only add to store positions if we actually have positions
    if (space_to_use > 0) {
        json_object_object_add(map_store_obj, "store_positions", store_positions);
        json_object_object_add(map_store_obj, "free_positions", updated_free_positions);
        json_object_object_add(map_store_obj, "used_space", json_object_new_int64(total_used));

        json_object_object_add(map_plan, store_id_str, map_store_obj);

    }

    free(store_id_str);
    return total_used;
}

int write_to_store(int data_fd, char *store_dir, json_object *data_locations) {
    int status = 0;

    char mapstore_path[BUFSIZ];
    FILE *mapstore = NULL;
    uint64_t arr_i = 0;
    json_object *location_array = NULL;
    uint64_t first;
    uint64_t final;
    uint64_t sector_size = 0;
    uint64_t bytes_read = 0;
    char buf[BUFSIZ];
    uint64_t total_stored = 0;
    uint64_t bytes_written = 0;
    uint64_t bytes_to_read = 0;

    json_object_object_foreach(data_locations, file, arr) {
        memset(mapstore_path, '\0', BUFSIZ);
        sprintf(mapstore_path, "%s%s.map", store_dir, file);
        mapstore = fopen(mapstore_path, "w");

        for (arr_i = 0; arr_i < json_object_array_length(arr); arr_i++) {
            location_array = json_object_array_get_idx(arr, arr_i);
            first = json_object_get_int64(json_object_array_get_idx(location_array, 0));
            final = json_object_get_int64(json_object_array_get_idx(location_array, 1));
            sector_size = final - first + 1;

            bytes_read = 0;
            bytes_written = 0;
            do {
                memset(buf, '\0', BUFSIZ);
                bytes_to_read = ((sector_size - bytes_written) > BUFSIZ) ? BUFSIZ : sector_size - bytes_written;
                bytes_read = pread(data_fd, buf, bytes_to_read, total_stored);

                bytes_written = pwrite(fileno(mapstore), buf, bytes_read, bytes_read + first);

                total_stored += bytes_written;
            } while (total_stored < sector_size);

            if (mapstore) {
                fclose(mapstore);
            }
        }
    }

    return status;
}
