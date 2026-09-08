/* Return Value */
#define DAMO_CRYPT_SUCCESS                        00000 /* Success */
#define DAMO_CRYPT_ERR_RNG_NULL_POINTER          -40000 /* 난수생성시 NULL 포인터 에러 */
#define DAMO_CRYPT_ERR_RNG_INTERNAL_INIT_FAIL    -40001 /* 난수생성시 내부 초기화 실패 */
#define DAMO_CRYPT_ERR_RNG_GEN_RAND_FAIL         -40002 /* 난수생성 실패 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EXPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllexport)
#elif defined(IMPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTCRYPT
#endif

/* DAMO_CRYPT_RNG
 * description : 난수 생성 함수
 *   
 * parameter
 * 1. out(OUT) : 생성된 난수를 저장할 포인터
 * 2. out_len(IN) : 생성할 난수의 길이
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_RNG( unsigned char *out, size_t out_len );

#ifdef __cplusplus
}
#endif

