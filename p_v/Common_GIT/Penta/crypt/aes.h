#include <string.h>
#include "dukpt_algo.h"

#if defined(EXPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllexport)
#elif defined(IMPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTCRYPT
#endif

#if defined(_MSC_VER) && !defined(EFIX64) && !defined(EFI32)
#include <basetsd.h>
typedef UINT32 uint32_t;
#else
#include <inttypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int nr;                     /*!<  number of rounds  */
    uint32_t *rk;               /*!<  AES round keys    */
    uint32_t buf[68];           /*!<  unaligned data    */
}
AES_CTX;

/* DAMO_CRYPT_AES_Encrypt
 * description : AES알고리즘 암호화 함수
 *   
 * parameter
 * 1. out(OUT) : 암호문 데이터를 저장할 포인터
 * 2. out_len(OUT) : 암호문 데이터 길이를 저장할 포인터
 * 3. in(IN) : 평문 데이터
 * 4. in_len(IN) : 평문 데이터 길이
 * 5. key(IN) : 입력 키
 * 6. key_len(IN) : 입력 키 길이
 * 7. alg(IN) : AES알고리즘 타입 (AES_128, AES_192, AES_256)
 * 8. mode(IN) : 블록암호 운영모드 (CBC_MODE , CFB_MODE)
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_AES_Encrypt(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode);

/* DAMO_CRYPT_AES_Decrypt
 * description : AES알고리즘 복호화 함수
 *   
 * parameter
 * 1. out(OUT) : 평문 데이터를 저장할 포인터
 * 2. out_len(OUT) : 평문 데이터 길이를 저장할 포인터
 * 3. in(IN) : 암호문 데이터
 * 4. in_len(IN) : 암호문 데이터 길이
 * 5. key(IN) : 입력 키
 * 6. key_len(IN) : 입력 키 길이
 * 7. alg(IN) : AES알고리즘 타입 (AES_128, AES_192, AES_256)
 * 8. mode(IN) : 블록암호 운영모드 (CBC_MODE , CFB_MODE)
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */

LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_AES_Decrypt(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode);

/* DAMO_CRYPT_AES_EncryptEx
 * description : AES알고리즘 암호화 함수, Initial Vector를 지정할 수 있다.
 *   
 * parameter
 * 1. out(OUT) : 암호문 데이터를 저장할 포인터
 * 2. out_len(OUT) : 암호문 데이터 길이를 저장할 포인터
 * 3. in(IN) : 평문 데이터
 * 4. in_len(IN) : 평문 데이터 길이
 * 5. key(IN) : 입력 키
 * 6. key_len(IN) : 입력 키 길이
 * 7. alg(IN) : AES알고리즘 타입 (AES_128, AES_192, AES_256)
 * 8. mode(IN) : 블록암호 운영모드 (CBC_MODE , CFB_MODE)
 * 9. ivec(IN) : 블록암호 Initial Vector ( 16byte )
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_AES_EncryptEx(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec);

/* DAMO_CRYPT_AES_DecryptEx
 * description : AES알고리즘 복호화 함수, Initial Vector를 지정할 수 있다.
 *   
 * parameter
 * 1. out(OUT) : 평문 데이터를 저장할 포인터
 * 2. out_len(OUT) : 평문 데이터 길이를 저장할 포인터
 * 3. in(IN) : 암호문 데이터
 * 4. in_len(IN) : 암호문 데이터 길이
 * 5. key(IN) : 입력 키
 * 6. key_len(IN) : 입력 키 길이
 * 7. alg(IN) : AES알고리즘 타입 (AES_128, AES_192, AES_256)
 * 8. mode(IN) : 블록암호 운영모드 (CBC_MODE , CFB_MODE)
 * 9. ivec(IN) : 블록암호 Initial Vector ( 16byte )
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */

LIBSPEC_DUKPTCRYPT
int DAMO_CRYPT_AES_DecryptEx(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec);
#ifdef __cplusplus
}
#endif


