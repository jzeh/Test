#include "sha256.h"

/* Return Value */
#define DAMO_CRYPT_SUCCESS                        00000 /* Success */
#define DAMO_CRYPT_ERR_HMAC_SHA256_NULL_POINTER  -30100 /* HMAC_SHA256계산시 NULL 포인터 에러 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	HASH_SHA256_CTX hash_ctx;
	unsigned char ipad[64]; /* inner padding */
	unsigned char opad[64]; /* outer padding */
} HMAC_SHA256_CTX;

/* DAMO_CRYPT_HMAC_SHA256
 * description : SHA256기반 HMAC 알고리즘 함수
 *   
 * parameter
 * 1. key(IN) : 입력 키
 * 2. key_len(IN) : 입력 키 길이
 * 3. data(IN) : 입력 데이터 길이
 * 4. data_len(IN) : 입력 데이터 길이
 * 5. digest(OUT) : 다이제스트 결과를 저장할 포인터
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_CRYPT_HMAC_SHA256(const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len, unsigned char *digest);

#ifdef __cplusplus
}
#endif
