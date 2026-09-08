#ifndef DES_H
#define DES_H

#include "dukpt_algo.h"
#include <string.h>

#if defined(_MSC_VER) && !defined(EFIX64) && !defined(EFI32)
#include <basetsd.h>
typedef UINT32 uint32_t;
#else
#include <inttypes.h>
#endif

#if defined(EXPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllexport)
#elif defined(IMPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTCRYPT
#endif

#define DAMO_CRYPT_DES_ENC     1
#define DAMO_CRYPT_DES_DEC     0

#define DES_KEY_SIZE    8

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int is_enc;
    uint32_t sk[32];
}
DES_CTX;

typedef struct
{
    int is_enc;
    uint32_t sk[96];
}
DES3_CTX;

int DAMO_CRYPT_DES_Set_EKey( DES_CTX *ctx, const unsigned char key[DES_KEY_SIZE] );

int DAMO_CRYPT_DES_Set_DKey( DES_CTX *ctx, const unsigned char key[DES_KEY_SIZE] );

int DAMO_CRYPT_DES3_Set_EKey2( DES3_CTX *ctx, const unsigned char key[DES_KEY_SIZE * 2] );

int DAMO_CRYPT_DES3_Set_DKey2( DES3_CTX *ctx, const unsigned char key[DES_KEY_SIZE * 2] );

int DAMO_CRYPT_DES_Block( DES_CTX *ctx, const unsigned char input[8], unsigned char output[8] );

int DAMO_CRYPT_DES3_Block( DES3_CTX *ctx, const unsigned char input[8], unsigned char output[8] );

int DAMO_CRYPT_DES3_CBC( DES3_CTX *ctx, int is_enc, unsigned char iv[8],
		const unsigned char *input, size_t input_len, unsigned char *output );

#ifdef __cplusplus
}
#endif

#endif

