#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "git_sha1.h"
#include "types.h"
#include "compiler_port.h"

#define rol(value, bits)   (((value) << (bits)) | ((value) >> (32 - (bits))))

#define K0  0x5A827999UL
#define K1  0x6ED9EBA1UL
#define K2  0x8F1BBCDCUL
#define K3  0xCA62C1D6UL

#define ITERATIONS 4096

static void SHA1Transform(uint32_t *state, const uint8_t *buffer);
void gitSHA1Init(SHA1_CTX *context);
void gitSHA1Update(SHA1_CTX *context, const uint8_t *data, size_t len);
void gitSHA1Final(uint8_t *digest, SHA1_CTX *context);
void hmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *digest);
void pbkdf2_hmac_sha1(const uint8_t *password, size_t password_len, const uint8_t *salt, size_t salt_len, uint8_t *output, size_t dkLen);


static void SHA1Transform(uint32_t *state, const uint8_t *buffer)
{
    uint32_t a, b, c, d, e, temp, W[80];
    int t;

    for (t = 0; t < 16; t++)
	{
        W[t] = ((uint32_t)buffer[t * 4]) << 24 |
               ((uint32_t)buffer[t * 4 + 1]) << 16 |
               ((uint32_t)buffer[t * 4 + 2]) << 8 |
               ((uint32_t)buffer[t * 4 + 3]);
    }
    for (t = 16; t < 80; t++)
	{
        W[t] = rol(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    for (t = 0; t < 80; t++)
	{
        if (t < 20)
            temp = rol(a, 5) + ((b & c) | ((~b) & d)) + e + W[t] + K0;
        else if (t < 40)
            temp = rol(a, 5) + (b ^ c ^ d) + e + W[t] + K1;
        else if (t < 60)
            temp = rol(a, 5) + ((b & c) | (b & d) | (c & d)) + e + W[t] + K2;
        else
            temp = rol(a, 5) + (b ^ c ^ d) + e + W[t] + K3;
        e = d;
        d = c;
        c = rol(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void gitSHA1Init(SHA1_CTX *context)
{
    context->state[0] = 0x67452301UL;
    context->state[1] = 0xEFCDAB89UL;
    context->state[2] = 0x98BADCFEUL;
    context->state[3] = 0x10325476UL;
    context->state[4] = 0xC3D2E1F0UL;
    context->count = 0;
}

void gitSHA1Update(SHA1_CTX *context, const uint8_t *data, size_t len)
{
    size_t i, j;
	
    j = (size_t)((context->count >> 3) & 63);
    context->count += ((uint64_t)len) << 3;
	
    for (i = 0; i < len; i++)
	{
        context->buffer[j++] = data[i];
        if (j == SHA1_BLOCK_SIZE)
		{
            SHA1Transform(context->state, context->buffer);
            j = 0;
        }
    }
}

void gitSHA1Final(uint8_t *digest, SHA1_CTX *context)
{
    uint8_t finalcount[8];
    uint8_t c;
    size_t i;
    for (i = 0; i < 8; i++)
	{
        finalcount[i] = (uint8_t)((context->count >> ((7 - i) * 8)) & 0xFF);
    }
	
    c = 0x80;
    gitSHA1Update(context, &c, 1);
	
    while ((context->count >> 3) % SHA1_BLOCK_SIZE != 56)
	{
        c = 0x00;
        gitSHA1Update(context, &c, 1);
    }
	
    gitSHA1Update(context, finalcount, 8);
	
    for (i = 0; i < SHA1_DIGEST_SIZE; i++)
	{
        digest[i] = (uint8_t)((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);
    }
	
    memset(context, 0, sizeof(*context));
    memset(finalcount, 0, sizeof(finalcount));
}

/* HMAC-SHA1: SHA1을 이용하여 HMAC 계산 */
void hmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *digest)
{
    uint8_t k_ipad[SHA1_BLOCK_SIZE];
    uint8_t k_opad[SHA1_BLOCK_SIZE];
    uint8_t tk[SHA1_DIGEST_SIZE];
    size_t i;

    if (key_len > SHA1_BLOCK_SIZE)
	{
        SHA1_CTX tctx;
        gitSHA1Init(&tctx);
        gitSHA1Update(&tctx, key, key_len);
        gitSHA1Final(tk, &tctx);
        key = tk;
        key_len = SHA1_DIGEST_SIZE;
    }

    memset(k_ipad, 0, SHA1_BLOCK_SIZE);
    memset(k_opad, 0, SHA1_BLOCK_SIZE);
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    for (i = 0; i < SHA1_BLOCK_SIZE; i++)
	{
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5C;
    }

    SHA1_CTX context;
    uint8_t inner_digest[SHA1_DIGEST_SIZE];

    gitSHA1Init(&context);
    gitSHA1Update(&context, k_ipad, SHA1_BLOCK_SIZE);
    gitSHA1Update(&context, data, data_len);
    gitSHA1Final(inner_digest, &context);

    gitSHA1Init(&context);
    gitSHA1Update(&context, k_opad, SHA1_BLOCK_SIZE);
    gitSHA1Update(&context, inner_digest, SHA1_DIGEST_SIZE);
    gitSHA1Final(digest, &context);
}

/* PBKDF2-HMAC-SHA1: password, salt, 반복횟수를 이용하여 키 도출 */
void pbkdf2_hmac_sha1(const uint8_t *password, size_t password_len, const uint8_t *salt, size_t salt_len, uint8_t *output, size_t dkLen)
{
    uint32_t	block_count = (dkLen + SHA1_DIGEST_SIZE - 1) / SHA1_DIGEST_SIZE;
    uint8_t		U[SHA1_DIGEST_SIZE];
    uint8_t		T[SHA1_DIGEST_SIZE];
    uint8_t		salt_block[MAX_SALT_LENGTH + 4];
    uint32_t	i, j, k;

    memcpy(salt_block, salt, salt_len);

    for (i = 1; i <= block_count; i++)
	{
        salt_block[salt_len + 0] = (uint8_t)((i >> 24) & 0xFF);
        salt_block[salt_len + 1] = (uint8_t)((i >> 16) & 0xFF);
        salt_block[salt_len + 2] = (uint8_t)((i >> 8) & 0xFF);
        salt_block[salt_len + 3] = (uint8_t)(i & 0xFF);

        hmac_sha1(password, password_len, salt_block, salt_len + 4, U);
        memcpy(T, U, SHA1_DIGEST_SIZE);

        for (j = 1; j < ITERATIONS; j++)
		{
            hmac_sha1(password, password_len, U, SHA1_DIGEST_SIZE, U);
            for (k = 0; k < SHA1_DIGEST_SIZE; k++)
			{
                T[k] ^= U[k];
            }
        }

        size_t offset = (i - 1) * SHA1_DIGEST_SIZE;
        size_t r = (dkLen - offset < SHA1_DIGEST_SIZE) ? (dkLen - offset) : SHA1_DIGEST_SIZE;
        memcpy(output + offset, T, r);
    }
}
