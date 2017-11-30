#include <ctype.h>
#include <getopt.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <nettle/ripemd160.h>
#include <nettle/sha.h>
#include <nettle/base16.h>
#include <glob.h>

int get_file_hash(int fd, char **hash);
