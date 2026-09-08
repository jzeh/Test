#ifndef _DUKPT_SHA256_H
#define _DUKPT_SHA256_H

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

/* Return Value */
#define DAMO_CRYPT_SUCCESS                        00000 /* Success */
#define DAMO_CRYPT_ERR_HASH_SHA256_NULL_POINTER  -20100 /* HASH SHA256계산시 NULL 포인터 에러 */

#ifdef __cplusplus
extern "C" {
#endif

/* SHA256 Context */
typedef struct
{
    uint32_t total[2]; /* 처리될 바이트수 */
    uint32_t state[8]; /* 해쉬 중간 상태 */
    unsigned char buffer[64]; /* 처리될 데이터 블록 */
}
HASH_SHA256_CTX;

void DAMO_CRYPT_HASH_SHA256_Init( HASH_SHA256_CTX *ctx );

void DAMO_CRYPT_HASH_SHA256_Update( HASH_SHA256_CTX *ctx, const unsigned char *data, size_t data_len );

void DAMO_CRYPT_HASH_SHA256_Final( HASH_SHA256_CTX *ctx, unsigned char digest[32] );

/* DAMO_CRYPT_HASH_SHA256
 * description : SHA256 해쉬 알고리즘 함수
 * 
 * parameter
 * 1. data(IN) : 입력 데이터 포인터
 * 2. data_len(IN) : 입력 데이터 길이
 * 3. digest(OUT) : 다이제스트 결과를 저장할 포인터
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_HASH_SHA256( const unsigned char *data, size_t data_len, unsigned char digest[32] );

void *DAMO_CRYPT_KDF2_SHA256( const void *in, size_t in_len, void *out, size_t *out_len );

#ifdef __cplusplus
}
#endif

#endif /* _DUKPT_SHA256_H */

