#include "cli_helper.h"

int get_file_hash(int fd, char **hash) {
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
            fprintf(stderr, "Error reading file for hashing\n");
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
        fprintf(stderr, "Error converting hash data to string\n");
        return 1;
    }

    base16_encode_update(*hash, RIPEMD160_DIGEST_SIZE, hash_as_hex);

    return 0;
}
