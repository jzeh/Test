
#ifndef __GIT_AesEncrypt_H
#define __GIT_AesEncrypt_H

#ifndef AES256_CBC
#define AES256_CBC
#endif

#define AES256_ECB

#if defined(AES256_CBC)

/*********************************************************************
* Filename:   aes.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding AES implementation.
*********************************************************************/

//#ifndef AES_H
//#define AES_H

/*************************** HEADER FILES ***************************/
//#include "stdlib.h"
//#include "common.h"

#include "stddef.h"
#include "common.h"

/****************************** MACROS ******************************/
#define AES_BLOCK_SIZE 16               // AES operates on 16 bytes at a time

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif
/*********************** FUNCTION DECLARATIONS **********************/
///////////////////
// AES
///////////////////
// Key setup must be done before any AES en/de-cryption functions can be used.
void aes_key_setup(const BYTE key[],          // The key, must be 128, 192, or 256 bits
                   DWORD w[],                  // Output key schedule to be used later
                   int keysize);              // Bit length of the key, 128, 192, or 256

void aes_encrypt(const BYTE in[],             // 16 bytes of plaintext
                 BYTE out[],                  // 16 bytes of ciphertext
                 const DWORD key[],            // From the key setup
                 int keysize);                // Bit length of the key, 128, 192, or 256

void aes_decrypt(const BYTE in[],             // 16 bytes of ciphertext
                 BYTE out[],                  // 16 bytes of plaintext
                 const DWORD key[],            // From the key setup
                 int keysize);                // Bit length of the key, 128, 192, or 256

void aes_key_setup_new(const BYTE key[],          // The key, must be 128, 192, or 256 bits
                   DWORD w[],                  // Output key schedule to be used later
                   int keysize);              // Bit length of the key, 128, 192, or 256

void aes_encrypt_new(const BYTE in[],             // 16 bytes of plaintext
                 BYTE out[],                  // 16 bytes of ciphertext
                 const DWORD key[],            // From the key setup
                 int keysize);                // Bit length of the key, 128, 192, or 256

void aes_decrypt_new(const BYTE in[],             // 16 bytes of ciphertext
                 BYTE out[],                  // 16 bytes of plaintext
                 const DWORD key[],            // From the key setup
                 int keysize);                // Bit length of the key, 128, 192, or 256

///////////////////
// AES - CBC
///////////////////
int aes_encrypt_cbc(const BYTE in[],          // Plaintext
                    size_t in_len,            // Must be a multiple of AES_BLOCK_SIZE
                    BYTE out[],               // Ciphertext, same length as plaintext
                    const DWORD key[],         // From the key setup
                    int keysize,              // Bit length of the key, 128, 192, or 256
                    const BYTE iv[]);         // IV, must be AES_BLOCK_SIZE bytes long

// Only output the CBC-MAC of the input.
int aes_encrypt_cbc_mac(const BYTE in[],      // plaintext
                        size_t in_len,        // Must be a multiple of AES_BLOCK_SIZE
                        BYTE out[],           // Output MAC
                        const DWORD key[],     // From the key setup
                        int keysize,          // Bit length of the key, 128, 192, or 256
                        const BYTE iv[]);     // IV, must be AES_BLOCK_SIZE bytes long

int aes_encrypt_cbc_new(const BYTE in[],          // Plaintext
                    size_t in_len,            // Must be a multiple of AES_BLOCK_SIZE
                    BYTE out[],               // Ciphertext, same length as plaintext
                    const DWORD key[],         // From the key setup
                    int keysize,              // Bit length of the key, 128, 192, or 256
                    const BYTE iv[]);         // IV, must be AES_BLOCK_SIZE bytes long

int aes_decrypt_cbc(const BYTE in[], size_t in_len, BYTE out[], const DWORD key[], int keysize, const BYTE iv[]);
int aes_decrypt_cbc_new(const BYTE in[], size_t in_len, BYTE out[], const DWORD key[], int keysize, const BYTE iv[]);

///////////////////
// AES - CTR
///////////////////
void increment_iv(BYTE iv[],                  // Must be a multiple of AES_BLOCK_SIZE
                  int counter_size);          // Bytes of the IV used for counting (low end)

void aes_encrypt_ctr(const BYTE in[],         // Plaintext
                     size_t in_len,           // Any byte length
                     BYTE out[],              // Ciphertext, same length as plaintext
                     const DWORD key[],        // From the key setup
                     int keysize,             // Bit length of the key, 128, 192, or 256
                     const BYTE iv[]);        // IV, must be AES_BLOCK_SIZE bytes long

void aes_decrypt_ctr(const BYTE in[],         // Ciphertext
                     size_t in_len,           // Any byte length
                     BYTE out[],              // Plaintext, same length as ciphertext
                     const DWORD key[],        // From the key setup
                     int keysize,             // Bit length of the key, 128, 192, or 256
                     const BYTE iv[]);        // IV, must be AES_BLOCK_SIZE bytes long

///////////////////
// AES - CCM
///////////////////
// Returns True if the input parameters do not violate any constraint.
int aes_encrypt_ccm(const BYTE plaintext[],              // IN  - Plaintext.
                    DWORD plaintext_len,                  // IN  - Plaintext length.
                    const BYTE associated_data[],        // IN  - Associated Data included in authentication, but not encryption.
                    unsigned short associated_data_len,  // IN  - Associated Data length in bytes.
                    const BYTE nonce[],                  // IN  - The Nonce to be used for encryption.
                    unsigned short nonce_len,            // IN  - Nonce length in bytes.
                    BYTE ciphertext[],                   // OUT - Ciphertext, a concatination of the plaintext and the MAC.
                    DWORD *ciphertext_len,                // OUT - The length of the ciphertext, always plaintext_len + mac_len.
                    DWORD mac_len,                        // IN  - The desired length of the MAC, must be 4, 6, 8, 10, 12, 14, or 16.
                    const BYTE key[],                    // IN  - The AES key for encryption.
                    int keysize);                        // IN  - The length of the key in bits. Valid values are 128, 192, 256.

// Returns True if the input parameters do not violate any constraint.
// Use mac_auth to ensure decryption/validation was preformed correctly.
// If authentication does not succeed, the plaintext is zeroed out. To overwride
// this, call with mac_auth = NULL. The proper proceedure is to decrypt with
// authentication enabled (mac_auth != NULL) and make a second call to that
// ignores authentication explicitly if the first call failes.
int aes_decrypt_ccm(const BYTE ciphertext[],             // IN  - Ciphertext, the concatination of encrypted plaintext and MAC.
                    DWORD ciphertext_len,                 // IN  - Ciphertext length in bytes.
                    const BYTE assoc[],                  // IN  - The Associated Data, required for authentication.
                    unsigned short assoc_len,            // IN  - Associated Data length in bytes.
                    const BYTE nonce[],                  // IN  - The Nonce to use for decryption, same one as for encryption.
                    unsigned short nonce_len,            // IN  - Nonce length in bytes.
                    BYTE plaintext[],                    // OUT - The plaintext that was decrypted. Will need to be large enough to hold ciphertext_len - mac_len.
                    DWORD *plaintext_len,                 // OUT - Length in bytes of the output plaintext, always ciphertext_len - mac_len .
                    DWORD mac_len,                        // IN  - The length of the MAC that was calculated.
                    int *mac_auth,                       // OUT - TRUE if authentication succeeded, FALSE if it did not. NULL pointer will ignore the authentication.
                    const BYTE key[],                    // IN  - The AES key for decryption.
                    int keysize);                        // IN  - The length of the key in BITS. Valid values are 128, 192, 256.

///////////////////
// Test functions
///////////////////
int aes_test();
int aes_ecb_test();
int aes_cbc_test();
int aes_ctr_test();
int aes_ccm_test();

#endif   // AES_H

#if defined(AES256_ECB)
/*
*   Byte-oriented AES-256 implementation.
*   All lookup tables replaced with 'on the fly' calculations.
*
*   Copyright (c) 2007-2009 Ilya O. Levin, http://www.literatecode.com
*   Other contributors: Hal Finney
*
*   Permission to use, copy, modify, and distribute this software for any
*   purpose with or without fee is hereby granted, provided that the above
*   copyright notice and this permission notice appear in all copies.
*
*   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
*   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
*   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
*   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
*   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
*   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
*   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#ifndef uint8_t
#define uint8_t  unsigned char
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        uint8_t key[32];
        uint8_t enckey[32];
        uint8_t deckey[32];
    } aes256_context;


    void aes256_init(aes256_context *, uint8_t * /* key */);
    void aes256_done(aes256_context *);
    void aes256_encrypt_ecb(aes256_context *, uint8_t * /* plaintext */);
    void aes256_decrypt_ecb(aes256_context *, uint8_t * /* cipertext */);

#ifdef __cplusplus
}
#endif
#endif

#endif /* __GIT_AesEncrypt_H */