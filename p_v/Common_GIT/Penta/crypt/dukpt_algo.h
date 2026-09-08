#ifndef DUKPT_ALGO_H
#define DUKPT_ALGO_H

#include <stdio.h>

#if defined(_MSC_VER) && !defined(EFIX64) && !defined(EFI32)
#include <basetsd.h>
typedef UINT32 uint32_t;
#else
#include <inttypes.h>
#endif

#if __alpha__	||	__alpha	||	__i386__	||	i386	||	_M_I86	||	_M_IX86	||	\
	__OS2__		||	sun386	||	__TURBOC__	||	vax		||	vms		||	VMS		||	__VMS || __x86_64
#define LITTLE_ENDIAN_C
#else
#define BIG_ENDIAN_C
#endif

#if defined(EXPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllexport)
#elif defined(IMPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTCRYPT
#endif

#define DAMO_CRYPT_ENC     1
#define DAMO_CRYPT_DEC     0

//#define INLENMAX 256
#define INLENMAX (3*1024)

/* Return Value */
#define DAMO_CRYPT_SUCCESS                        00000 /* Success */

#define DAMO_CRYPT_ERR_AES_ENC_NULL_POINTER       -10000 /* AES암호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_AES_ENC_INVALID_LEN        -10001 /* AES암호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_AES_ENC_INVALID_ALG        -10002 /* AES암호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_AES_ENC_INVALID_MODE       -10003 /* AES암호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_AES_ENC_KEY_SCH_FAIL       -10004 /* AES암호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_AES_DEC_NULL_POINTER       -10005 /* AES복호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_AES_DEC_INVALID_LEN        -10006 /* AES복호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_AES_DEC_INVALID_ALG        -10007 /* AES복호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_AES_DEC_INVALID_MODE       -10008 /* AES복호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_AES_DEC_KEY_SCH_FAIL       -10009 /* AES복호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_AES_DEC_INVALID_PAD        -10010 /* AES복호화시 유효하지 않는 패딩 */
#define DAMO_CRYPT_ERR_AES_ENC_INVALID_PAD        -10011 /* AES암호화시 유효하지 않는 패딩 */
#define DAMO_CRYPT_ERR_AES_DEC_GEOMJEUNG_PAD      -10012 /* AES복호화시 패딩 검증 실패 */

#define DAMO_CRYPT_ERR_DES_ENC_NULL_POINTER       -10100 /* DES암호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_DES_ENC_INVALID_LEN        -10101 /* DES암호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_DES_ENC_INVALID_ALG        -10102 /* DES암호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_DES_ENC_INVALID_MODE       -10103 /* DES암호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_DES_ENC_KEY_SCH_FAIL       -10104 /* DES암호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_DES_DEC_NULL_POINTER       -10105 /* DES복호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_DES_DEC_INVALID_LEN        -10106 /* DES복호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_DES_DEC_INVALID_ALG        -10107 /* DES복호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_DES_DEC_INVALID_MODE       -10108 /* DES복호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_DES_DEC_KEY_SCH_FAIL       -10109 /* DES복호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_DES_DEC_INVALID_PAD        -10110 /* DES복호화시 유효하지 않는 패딩 */
#define DAMO_CRYPT_ERR_DES_ENC_INVALID_PAD        -10111 /* DES암호화시 유효하지 않는 패딩 */
#define DAMO_CRYPT_ERR_DES_DEC_GEOMJEUNG_PAD      -10112 /* DES복호화시 패딩 검증 실패 */

#define DAMO_CRYPT_ERR_SEED_ENC_NULL_POINTER      -10200 /* SEED암호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_SEED_ENC_INVALID_LEN       -10201 /* SEED암호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_SEED_ENC_INVALID_ALG       -10202 /* SEED암호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_SEED_ENC_INVALID_MODE      -10203 /* SEED암호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_SEED_ENC_KEY_SCH_FAIL      -10204 /* SEED암호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_SEED_DEC_NULL_POINTER      -10205 /* SEED복호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_SEED_DEC_INVALID_LEN       -10206 /* SEED복호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_SEED_DEC_INVALID_ALG       -10207 /* SEED복호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_SEED_DEC_INVALID_MODE      -10208 /* SEED복호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_SEED_DEC_KEY_SCH_FAIL      -10209 /* SEED복호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_SEED_DEC_INVALID_PAD       -10210 /* SEED복호화시 유효하지 패딩 */
#define DAMO_CRYPT_ERR_SEED_ENC_INVALID_PAD       -10211 /* SEED암호화시 유효하지 않는 패딩 */
#define DAMO_CRYPT_ERR_SEED_DEC_GEOMJEUNG_PAD     -10212 /* SEED복호화시 패딩 검증 실패 */

#define DAMO_CRYPT_ERR_HIGHT_ENC_NULL_POINTER     -10300 /* HIGHT암호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_HIGHT_ENC_INVALID_LEN      -10301 /* HIGHT암호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_HIGHT_ENC_INVALID_ALG      -10302 /* HIGHT암호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_HIGHT_ENC_INVALID_MODE     -10303 /* HIGHT암호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_HIGHT_ENC_KEY_SCH_FAIL     -10304 /* HIGHT암호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_HIGHT_DEC_NULL_POINTER     -10305 /* HIGHT복호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_HIGHT_DEC_INVALID_LEN      -10306 /* HIGHT복호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_HIGHT_DEC_INVALID_ALG      -10307 /* HIGHT복호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_HIGHT_DEC_INVALID_MODE     -10308 /* HIGHT복호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_HIGHT_DEC_KEY_SCH_FAIL     -10309 /* HIGHT복호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_HIGHT_DEC_INVALID_PAD      -10310 /* HIGHT복호화시 유효하지 패딩 */
#define DAMO_CRYPT_ERR_HIGHT_ENC_INVALID_PAD      -10311 /* HIGHT암호화시 유효하지 않는 패딩 */
#define DAMO_CRYPT_ERR_HIGHT_DEC_GEOMJEUNG_PAD    -10312 /* HIGHT복호화시 패딩 검증 실패 */

#define DAMO_CRYPT_ERR_TDES_ENC_NULL_POINTER      -10400 /* TDES암호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_TDES_ENC_INVALID_LEN       -10401 /* TDES암호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_TDES_ENC_INVALID_ALG       -10402 /* TDES암호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_TDES_ENC_INVALID_MODE      -10403 /* TDES암호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_TDES_ENC_KEY_SCH_FAIL      -10404 /* TDES암호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_TDES_DEC_NULL_POINTER      -10405 /* TDES복호화시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_TDES_DEC_INVALID_LEN       -10406 /* TDES복호화시 유효하지 않는 길이 */
#define DAMO_CRYPT_ERR_TDES_DEC_INVALID_ALG       -10407 /* TDES복호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_TDES_DEC_INVALID_MODE      -10408 /* TDES복호화시 유효하지 않는 운용모드 */
#define DAMO_CRYPT_ERR_TDES_DEC_KEY_SCH_FAIL      -10409 /* TDES복호화시 키스케쥴링 실패 */
#define DAMO_CRYPT_ERR_TDES_DEC_INVALID_PAD       -10410 /* TDES복호화시 유효하지 패딩 */
#define DAMO_CRYPT_ERR_TDES_ENC_INVALID_PAD       -10411 /* TDES암호화시 유효하지 않는 패딩 */
#define DAMO_CRYPT_ERR_TDES_DEC_GEOMJEUNG_PAD     -10412 /* TDES복호화시 패딩 검증 실패 */

#define DAMO_CRYPT_ERR_ENC_INVALID_ALG            -10900 /* 암호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_DEC_INVALID_ALG            -10901 /* 복호화시 유효하지 않는 알고리즘 */
#define DAMO_CRYPT_ERR_MALLOC                     -10902 /* 메모리 할당 실패 */
#define DAMO_CRYPT_ERR_INVALID_HEX_DATA           -10903 /* HEX STRING 데이터가 맞지 않음 */

//typedef unsigned long       DWORD;
//typedef unsigned int        UINT;
//typedef unsigned short      WORD;
//typedef unsigned char       BYTE;

typedef enum {
	AES_128,
	AES_192,
	AES_256,
	SEED_128,
	HIGHT,
	TDES
} ALGO_TYPE;

typedef enum {
	CBC_MODE,
	CFB_MODE
} MODE_TYPE;

LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_EncryptEx(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, size_t pad);
LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_DecryptEx(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, size_t pad);
#endif

