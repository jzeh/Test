#ifndef GIT_SHA1_H
#define GIT_SHA1_H

#include <stddef.h>
#include <stdint.h>

#define SHA1_BLOCK_SIZE    64
#define SHA1_DIGEST_SIZE   20
#define MAX_SALT_LENGTH    64

typedef struct {
    uint32_t state[5];
    uint64_t count;
    uint8_t buffer[SHA1_BLOCK_SIZE];
} SHA1_CTX;

void gitSHA1Init(SHA1_CTX *context);
void gitSHA1Update(SHA1_CTX *context, const uint8_t *data, size_t len);
void gitSHA1Final(uint8_t *digest, SHA1_CTX *context);
void hmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *digest);
void pbkdf2_hmac_sha1(const uint8_t *password, size_t password_len, const uint8_t *salt, size_t salt_len, uint8_t *output, size_t dkLen);

#endif /* GIT_SHA1_H */
