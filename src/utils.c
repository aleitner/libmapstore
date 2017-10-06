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

int get_file_hash(int fd, uint8_t **hash) {
    ssize_t read_len = 0;
    uint8_t read_data[BUFSIZ];
    uint8_t prehash_sha256[SHA256_DIGEST_SIZE];
    uint8_t hash_as_hex[RIPEMD160_DIGEST_SIZE];

    struct sha256_ctx sha256ctx;
    sha256_init(&sha256ctx);

    struct ripemd160_ctx ripemd160ctx;
    ripemd160_init(&ripemd160ctx);

    do {
        read_len = read(fd, read_data, BUFSIZ);

        if (read_len == -1) {
            printf("Error reading file for hashing\n");
            return 1;
        }

        sha256_update(&sha256ctx, read_len, read_data);

        memset(read_data, '\0', BUFSIZ);
    } while (read_len > 0);

    memset(prehash_sha256, '\0', SHA256_DIGEST_SIZE);
    sha256_digest(&sha256ctx, SHA256_DIGEST_SIZE, prehash_sha256);

    ripemd160_update(&ripemd160ctx, SHA256_DIGEST_SIZE, prehash_sha256);
    memset(hash_as_hex, '\0', RIPEMD160_DIGEST_SIZE);
    ripemd160_digest(&ripemd160ctx, RIPEMD160_DIGEST_SIZE, hash_as_hex);

    size_t encode_len = BASE16_ENCODE_LENGTH(RIPEMD160_DIGEST_SIZE);
    *hash = calloc(encode_len + 1, sizeof(uint8_t));
    if (!*hash) {
        printf("Error converting hash data to string\n");
        return 1;
    }

    base16_encode_update(*hash, RIPEMD160_DIGEST_SIZE, hash_as_hex);

    return 0;
}
