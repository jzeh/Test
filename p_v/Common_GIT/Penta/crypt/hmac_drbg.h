#include "hmac.h"

#ifndef DAMO_CRYPT_HMAC_DRBG_H
#define DAMO_CRYPT_HMAC_DRBG_H

/* Return Value */
#define DAMO_CRYPT_SUCCESS                        			   00000 /* Success */
#define DAMO_CRYPT_ERR_HMAC_DRBG_REQUEST_TOO_BIG              -40100 /* 한번의 호출의 과도한 랜덤 요청 */
#define DAMO_CRYPT_ERR_HMAC_DRBG_INPUT_TOO_BIG                -40101 /* 과도한 입력 */
#define DAMO_CRYPT_ERR_HMAC_DRBG_FILE_IO_ERROR                -40102 /* 파일에서 읽기/쓰기 에러 */
#define DAMO_CRYPT_ERR_HMAC_DRBG_ENTROPY_SOURCE_FAILED        -40103 /* 엔트로피 소스 실패 */

#define DAMO_CRYPT_HMAC_DRBG_MAX_INPUT_BYTES   256     /* 추가적인 Input 최대 바이트수 */
#define DAMO_CRYPT_HMAC_DRBG_MAX_REQUEST       1024    /* 호출시 Request 최대 바이트수 */
#define DAMO_CRYPT_HMAC_DRBG_MAX_SEED_BYTES    384     /* Reseed 최대 바이트수 */

#define DAMO_CRYPT_HMAC_DRBG_PR_OFF   0   /* No Prediction Resistance */
#define DAMO_CRYPT_HMAC_DRBG_PR_ON    1   /* Prediction Resistance Enabled */

#define DAMO_CRYPT_DIGEST_MAX_SIZE	  64

#ifdef __cplusplus
extern "C" {
#endif

/* HMAC DRBG Context */
typedef struct
{
    HMAC_CTX hmac_ctx;                    					/* HMAC Context */
    unsigned char V[DAMO_CRYPT_DIGEST_MAX_SIZE];
    int reseed_counter;                   					/* Reseed Counter */
    size_t entropy_len;         							/* 각 Seed에서 수집된 엔트로피 바이트 */
    int prediction_resistance;  							/* 예측 저항 기능 활성화(모든 랜덤 생성 전에 자동적인 Reseed) */
    int reseed_interval;        							/* Reseed Interval */
    int (*entropy_func)(void *, unsigned char *, size_t);   /* 엔트로피 함수 */
    void *entropy_ctx;            							/* 엔트로피 함수에 대한 Context */
} HMAC_DRBG_CTX;


/* DAMO_CRYPT_HMAC_DRBG_Update
 * description : HMAC DRBG State 업데이트 함수
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 * 2. add_data(IN) : State를 업데이트할 추가적인 데이터
 * 3. add_data_len(IN) : State를 업데이트할 추가적인 데이터 길이
 *
 * return value
 *  no
 */
void DAMO_CRYPT_HMAC_DRBG_Update( HMAC_DRBG_CTX *ctx, const unsigned char *add_data, size_t add_data_len );

/* DAMO_CRYPT_HMAC_DRBG_Reseed
 * description : HMAC DRBG Reseeding 함수(엔트로피 소스로부터 데이터 추출)
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 * 2. add_data(IN) : 추가적인 데이터
 * 3. add_data_len(IN) : 추가적인 데이터 길이
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_CRYPT_HMAC_DRBG_Reseed( HMAC_DRBG_CTX *ctx, const unsigned char *add_data, size_t add_data_len );

/* DAMO_CRYPT_HMAC_DRBG_Init
 * description : HMAC DRBG 초기화 함수
 *
 * parameter
 * 1. ctx(OUT) : 초기화할 HMAC DRBG Context 포인터
 * 2. hmac_type(IN) : HMAC 알고리즘 타입
 * 3. entropy_func(IN) : 엔트로피 수집 함수
 * 4. entropy_ctx(IN) : 엔트로피 수집 함수 Context
 * 5. add_data(IN) : 추가적인 데이터(e.g. Device specific identifiers)
 * 6. add_data_len(IN) : 추가적인 데이터 길이
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_CRYPT_HMAC_DRBG_Init( HMAC_DRBG_CTX *ctx, HMAC_TYPE hmac_type,
		int (*entropy_func)(void *, unsigned char *, size_t), void *entropy_ctx,
		const unsigned char *add_data, size_t add_data_len );

/* DAMO_CRYPT_HMAC_DRBG_Destroy
 * description : HMAC DRBG Context 반환 함수
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 *
 * return value
 *  no 
 */
void DAMO_CRYPT_HMAC_DRBG_Destroy( HMAC_DRBG_CTX *ctx );

/* DAMO_CRYPT_HMAC_DRBG_Random_With_Add
 * description : HMAC DRBG 난수생성 함수(추가적인 데이터를 입력)
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 * 2. output(OUT) : 난수 데이터를 채울 버퍼
 * 3. output_len(IN) : 버퍼의 길이
 * 4. add_data(IN) : 추가적인 데이터
 * 5. add_data_len(IN) : 추가적인 데이터 길이
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_CRYPT_HMAC_DRBG_Random_With_Add( HMAC_DRBG_CTX *ctx,
		unsigned char *output, size_t output_len,
		const unsigned char *add_data, size_t add_data_len );

/* DAMO_CRYPT_HMAC_DRBG_Random_Without_Add
 * description : HMAC DRBG 난수생성 함수(추가적인 데이터를 입력하지 않음)
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 * 2. output(OUT) : 난수 데이터를 채울 버퍼
 * 3. output_len(IN) : 버퍼의 길이
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_CRYPT_HMAC_DRBG_Random_Without_Add( HMAC_DRBG_CTX *ctx,
		unsigned char *output, size_t output_len);

/* DAMO_CRYPT_HMAC_DRBG_Set_Prediction_Resistance
 * description : Prediction Resistance 설정 함수 (Default: Off)
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 * 2. resistance(IN) : Prediction Resistance 설정값
 *
 * return value
 *  no 
 */
void DAMO_CRYPT_HMAC_DRBG_Set_Prediction_Resistance( HMAC_DRBG_CTX *ctx, int resistance );

/* DAMO_CRYPT_HMAC_DRBG_Set_Entropy_Len 
 * description : 엔트로피 길이 설정 함수
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 * 2. len(IN) : 엔트로피 길이 설정값
 *
 * return value
 *  no 
 */
void DAMO_CRYPT_HMAC_DRBG_Set_Entropy_Len( HMAC_DRBG_CTX *ctx, size_t len );

/* DAMO_CRYPT_HMAC_DRBG_Set_Reseed_Interval 
 * description : Reseed 주기 설정 함수
 *
 * parameter
 * 1. ctx(IN/OUT) : HMAC DRBG Context 포인터
 * 2. interval(IN) : Reseed 주기 설정값
 *
 * return value
 *  no 
 */
void DAMO_CRYPT_HMAC_DRBG_Set_Reseed_Interval( HMAC_DRBG_CTX *ctx, int interval );

#ifdef __cplusplus
}
#endif

#endif
