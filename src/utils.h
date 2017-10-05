/**
 * @file utils.h
 * @brief Map Store utilities.
 *
 * Helper utilities
 */
#ifndef MAPSTORE_UTILS_H
#define MAPSTORE_UTILS_H
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <nettle/base16.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#include <time.h>
#include <io.h>

ssize_t pread(int fd, void *buf, size_t count, uint64_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, uint64_t offset);

#else
#include <sys/time.h>
#include <sys/mman.h>
#endif

int allocatefile(int fd, uint64_t length);

int unmap_file(uint8_t *map, uint64_t filesize);

int map_file(int fd, uint64_t filesize, uint8_t **map, bool read_only);

#endif /* MAPSTORE_UTILS_H */
