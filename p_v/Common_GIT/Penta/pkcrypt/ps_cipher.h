/**
 * \file cipher.h
 *
 * \brief Generic cipher wrapper.
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright (C) 2006-2014, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://polarssl.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef POLARSSL_CIPHER_H
#define POLARSSL_CIPHER_H

#if !defined(POLARSSL_CONFIG_FILE)
#include "ps_config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_CIPHER_MODE_CBC)
#define POLARSSL_CIPHER_MODE_WITH_PADDING
#endif

#include <string.h>

#if defined(_MSC_VER) && !defined(inline)
#define inline _inline
#else
#if defined(__ARMCC_VERSION) && !defined(inline)
#define inline __inline
#endif /* __ARMCC_VERSION */
#endif /*_MSC_VER */

#define POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE            -0x6080  /**< The selected feature is not available. */
#define POLARSSL_ERR_CIPHER_BAD_INPUT_DATA                 -0x6100  /**< Bad input parameters to function. */
#define POLARSSL_ERR_CIPHER_ALLOC_FAILED                   -0x6180  /**< Failed to allocate memory. */
#define POLARSSL_ERR_CIPHER_INVALID_PADDING                -0x6200  /**< Input data contains invalid padding and is rejected. */
#define POLARSSL_ERR_CIPHER_FULL_BLOCK_EXPECTED            -0x6280  /**< Decryption of block requires a full block. */
#define POLARSSL_ERR_CIPHER_AUTH_FAILED                    -0x6300  /**< Authentication failed (for AEAD modes). */

#define POLARSSL_CIPHER_VARIABLE_IV_LEN     0x01    /**< Cipher accepts IVs of variable length */
#define POLARSSL_CIPHER_VARIABLE_KEY_LEN    0x02    /**< Cipher accepts keys of variable length */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POLARSSL_CIPHER_ID_NONE = 0,
    POLARSSL_CIPHER_ID_NULL,
    POLARSSL_CIPHER_ID_AES,
    POLARSSL_CIPHER_ID_DES,
    POLARSSL_CIPHER_ID_3DES,
    POLARSSL_CIPHER_ID_CAMELLIA,
    POLARSSL_CIPHER_ID_BLOWFISH,
    POLARSSL_CIPHER_ID_ARC4,
} cipher_id_t;

typedef enum {
    POLARSSL_CIPHER_NONE = 0,
    POLARSSL_CIPHER_NULL,
    POLARSSL_CIPHER_AES_128_ECB,
    POLARSSL_CIPHER_AES_192_ECB,
    POLARSSL_CIPHER_AES_256_ECB,
    POLARSSL_CIPHER_AES_128_CBC,
    POLARSSL_CIPHER_AES_192_CBC,
    POLARSSL_CIPHER_AES_256_CBC,
    POLARSSL_CIPHER_AES_128_CFB128,
    POLARSSL_CIPHER_AES_192_CFB128,
    POLARSSL_CIPHER_AES_256_CFB128,
    POLARSSL_CIPHER_AES_128_CTR,
    POLARSSL_CIPHER_AES_192_CTR,
    POLARSSL_CIPHER_AES_256_CTR,
    POLARSSL_CIPHER_AES_128_GCM,
    POLARSSL_CIPHER_AES_192_GCM,
    POLARSSL_CIPHER_AES_256_GCM,
    POLARSSL_CIPHER_CAMELLIA_128_ECB,
    POLARSSL_CIPHER_CAMELLIA_192_ECB,
    POLARSSL_CIPHER_CAMELLIA_256_ECB,
    POLARSSL_CIPHER_CAMELLIA_128_CBC,
    POLARSSL_CIPHER_CAMELLIA_192_CBC,
    POLARSSL_CIPHER_CAMELLIA_256_CBC,
    POLARSSL_CIPHER_CAMELLIA_128_CFB128,
    POLARSSL_CIPHER_CAMELLIA_192_CFB128,
    POLARSSL_CIPHER_CAMELLIA_256_CFB128,
    POLARSSL_CIPHER_CAMELLIA_128_CTR,
    POLARSSL_CIPHER_CAMELLIA_192_CTR,
    POLARSSL_CIPHER_CAMELLIA_256_CTR,
    POLARSSL_CIPHER_CAMELLIA_128_GCM,
    POLARSSL_CIPHER_CAMELLIA_192_GCM,
    POLARSSL_CIPHER_CAMELLIA_256_GCM,
    POLARSSL_CIPHER_DES_ECB,
    POLARSSL_CIPHER_DES_CBC,
    POLARSSL_CIPHER_DES_EDE_ECB,
    POLARSSL_CIPHER_DES_EDE_CBC,
    POLARSSL_CIPHER_DES_EDE3_ECB,
    POLARSSL_CIPHER_DES_EDE3_CBC,
    POLARSSL_CIPHER_BLOWFISH_ECB,
    POLARSSL_CIPHER_BLOWFISH_CBC,
    POLARSSL_CIPHER_BLOWFISH_CFB64,
    POLARSSL_CIPHER_BLOWFISH_CTR,
    POLARSSL_CIPHER_ARC4_128,
    POLARSSL_CIPHER_AES_128_CCM,
    POLARSSL_CIPHER_AES_192_CCM,
    POLARSSL_CIPHER_AES_256_CCM,
    POLARSSL_CIPHER_CAMELLIA_128_CCM,
    POLARSSL_CIPHER_CAMELLIA_192_CCM,
    POLARSSL_CIPHER_CAMELLIA_256_CCM,
} cipher_type_t;

typedef enum {
    POLARSSL_MODE_NONE = 0,
    POLARSSL_MODE_ECB,
    POLARSSL_MODE_CBC,
    POLARSSL_MODE_CFB,
    POLARSSL_MODE_OFB, /* Unused! */
    POLARSSL_MODE_CTR,
    POLARSSL_MODE_GCM,
    POLARSSL_MODE_STREAM,
    POLARSSL_MODE_CCM,
} cipher_mode_t;

typedef enum {
    POLARSSL_PADDING_PKCS7 = 0,     /**< PKCS7 padding (default)        */
    POLARSSL_PADDING_ONE_AND_ZEROS, /**< ISO/IEC 7816-4 padding         */
    POLARSSL_PADDING_ZEROS_AND_LEN, /**< ANSI X.923 padding             */
    POLARSSL_PADDING_ZEROS,         /**< zero padding (not reversible!) */
    POLARSSL_PADDING_NONE,          /**< never pad (full blocks only)   */
} cipher_padding_t;

typedef enum {
    POLARSSL_OPERATION_NONE = -1,
    POLARSSL_DECRYPT = 0,
    POLARSSL_ENCRYPT,
} operation_t;

enum {
    /** Undefined key length */
    POLARSSL_KEY_LENGTH_NONE = 0,
    /** Key length, in bits (including parity), for DES keys */
    POLARSSL_KEY_LENGTH_DES  = 64,
    /** Key length, in bits (including parity), for DES in two key EDE */
    POLARSSL_KEY_LENGTH_DES_EDE = 128,
    /** Key length, in bits (including parity), for DES in three-key EDE */
    POLARSSL_KEY_LENGTH_DES_EDE3 = 192,
};

/** Maximum length of any IV, in bytes */
#define POLARSSL_MAX_IV_LENGTH      16
/** Maximum block size of any cipher, in bytes */
#define POLARSSL_MAX_BLOCK_LENGTH   16

/**
 * Base cipher information. The non-mode specific functions and values.
 */
typedef struct {

    /** Base Cipher type (e.g. POLARSSL_CIPHER_ID_AES) */
    cipher_id_t cipher;

    /** Encrypt using ECB */
    int (*ecb_func)( void *ctx, operation_t mode,
                     const unsigned char *input, unsigned char *output );

    /** Encrypt using CBC */
    int (*cbc_func)( void *ctx, operation_t mode, size_t length,
                     unsigned char *iv, const unsigned char *input,
                     unsigned char *output );

    /** Encrypt using CFB (Full length) */
    int (*cfb_func)( void *ctx, operation_t mode, size_t length, size_t *iv_off,
                     unsigned char *iv, const unsigned char *input,
                     unsigned char *output );

    /** Encrypt using CTR */
    int (*ctr_func)( void *ctx, size_t length, size_t *nc_off,
                     unsigned char *nonce_counter, unsigned char *stream_block,
                     const unsigned char *input, unsigned char *output );

    /** Encrypt using STREAM */
    int (*stream_func)( void *ctx, size_t length,
                        const unsigned char *input, unsigned char *output );

    /** Set key for encryption purposes */
    int (*setkey_enc_func)( void *ctx, const unsigned char *key,
                            unsigned int key_length );

    /** Set key for decryption purposes */
    int (*setkey_dec_func)( void *ctx, const unsigned char *key,
                            unsigned int key_length);

    /** Allocate a new context */
    void * (*ctx_alloc_func)( void );

    /** Free the given context */
    void (*ctx_free_func)( void *ctx );

} cipher_base_t;

/**
 * Cipher information. Allows cipher functions to be called in a generic way.
 */
typedef struct {
    /** Full cipher identifier (e.g. POLARSSL_CIPHER_AES_256_CBC) */
    cipher_type_t type;

    /** Cipher mode (e.g. POLARSSL_MODE_CBC) */
    cipher_mode_t mode;

    /** Cipher key length, in bits (default length for variable sized ciphers)
     *  (Includes parity bits for ciphers like DES) */
    unsigned int key_length;

    /** Name of the cipher */
    const char * name;

    /** IV/NONCE size, in bytes.
     *  For cipher that accept many sizes: recommended size */
    unsigned int iv_size;

    /** Flags for variable IV size, variable key size, etc. */
    int flags;

    /** block size, in bytes */
    unsigned int block_size;

    /** Base cipher information and functions */
    const cipher_base_t *base;

} cipher_info_t;

/**
 * Generic cipher context.
 */
typedef struct {
    /** Information about the associated cipher */
    const cipher_info_t *cipher_info;

    /** Key length to use */
    int key_length;

    /** Operation that the context's key has been initialised for */
    operation_t operation;

    /** Padding functions to use, if relevant for cipher mode */
    void (*add_padding)( unsigned char *output, size_t olen, size_t data_len );
    int (*get_padding)( unsigned char *input, size_t ilen, size_t *data_len );

    /** Buffer for data that hasn't been encrypted yet */
    unsigned char unprocessed_data[POLARSSL_MAX_BLOCK_LENGTH];

    /** Number of bytes that still need processing */
    size_t unprocessed_len;

    /** Current IV or NONCE_COUNTER for CTR-mode */
    unsigned char iv[POLARSSL_MAX_IV_LENGTH];

    /** IV size in bytes (for ciphers with variable-length IVs) */
    size_t iv_size;

    /** Cipher-specific context */
    void *cipher_ctx;
} cipher_context_t;

#endif
