#ifndef __DUKPT_CLI_H__
#define __DUKPT_CLI_H__

#include "dukpt.h"

#ifndef DUKPT_SERVER
#include "dukpt_cli_config.h"
#endif

/* #define _DEBUG
 */
#if defined(EXPORT_DUKPTCLIENT)
#define LIBSPEC_DUKPTCLIENT  __declspec(dllexport)
#elif defined(IMPORT_DUKPTCLIENT)
#define LIBSPEC_DUKPTCLIENTT  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTCLIENT
#endif

/* 클라이언트에서 DUKPT 키 초기화 및 일회성 키 생성시 에러 코드 */
#define DAMO_DUKPT_SUCCESS                         000000 /* Success */
#define DAMO_DUKPT_ERR_CLI_LIK_NULL_POINTER       -101000 /* DUKPT 초기화 키 로딩시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_CLI_LIK_INVALID_LEN        -101001 /* DUKPT 초기화 키 로딩시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_CLI_RPE_NULL_POINTER       -101002 /* DUKPT 일회성 키 생성시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_CLI_RPE_INVALID_LEN        -101003 /* DUKPT 일회성 키 생성시 유효하지 않는 길이 */

#define DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_START -1000000
#define DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_END   -2000000
#define DAMO_DUKPT_ERR_CLI_CODE_BASE_REQUEST_ENTRY -3000000

#define DAMO_DUKPT_ERR_CLI_INVALID_INPUT_LEN              -1 /* DUKPT 잘못된 입력 길이 */
#define DAMO_DUKPT_ERR_CLI_NULL_POINTER                   -2 /* DUKPT NULL 포인트 에러 */
#define DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA             -3

#define DAMO_CRYPT_ERR_CLI_PK_AUTH_END_VERIFY_MAC         -4
#define DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_KSN         -5
#define DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_IPEK_LENGTH -6
#define DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_HASH        -7

#define DAMO_DUKPT_ERR_CLI_MAXIMUM_COUNT                  -8
#define DAMO_DUKPT_WARNING_CLI_LIMIT_COUNT                -9

#define DAMO_DUKPT_ERR_CLI_NOT_SUPPORTED_DATATYPE         -10
#define DAMO_DUKPT_ERR_CLI_INVALID_MAC_DATA_LEN           -11
#define DAMO_DUKPT_ERR_CLI_INVALID_VERIFICATION_DATA_LEN  -12

#define DAMO_DUKPT_ERR_CLI_INSUFFICIENT_ALLOC_LEN         -13

#ifdef __cplusplus
extern "C" {
#endif

/* DAMO_DUKPT_Load_Initial_Key
 * description : DUKPT 초기화 키 로딩 함수
 *   
 * parameter
 * 1. ipek(IN) : 16바이트 초기 PIN 암호화 키
 * 2. ipek_len(IN) : 초기 PIN 암호화 키 길이
 * 3. ksn(IN) : 10 바이트 Key Serial Number ( [59비트 Initial Key Serial Number]||[21비트 0] )
 * 4. ksn_len(IN) : Key Serial Number 길이
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Load_Initial_Key(const unsigned char *ipek, size_t ipek_len,
		const unsigned char *ksn, size_t ksn_len);

/* DAMO_DUKPT_Request_Pin_Entry
 * description : 암호화된 PIN 데이터 생성 및 MAC Key/Data Encryption Key 생성 함수
 *   
 * parameter
 * 1. pin(IN) : 입력할 PIN 데이터
 * 2. pin_len(IN) : 입력할 PIN 데이터 길이
 * 3. account_num(IN) : 카드 번호 데이터
 * 4. account_num_len(IN) : 카드 번호 데이터 길이
 * 5. flags(IN) : PIN 암호화 키 / Data MAC 키 / Data 암호화 키 사용여부 플래그
 * 6. pEntry(OUT) : 암호화된 PIN 데이터 및 MAC KEY 그리고 Data Encryption Key를 저장할 구조체 포인터
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Request_Pin_Entry(const unsigned char *pin, size_t pin_len,
		const unsigned char *account_num, size_t account_num_len, int flags, DukptPinEntry *pEntry);

/* DAMO_DUKPT_Export_Future_Key_Info
 * description : KSN과 Future Key Set을 가져오는 함수
 *   
 * parameter
 * 1. fki(OUT) : KSN과 Future Key Set를 저장할 구조체 포인터 
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Export_Future_Key_Info(DukptFutureKeyInfo *fki);
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Export_Future_Key_Info_Ek(unsigned char *external_key, DukptFutureKeyInfo *fki);

/* DAMO_DUKPT_Import_Future_Key_Info
 * description : KSN과 Future Key Set을 설정하는 함수
 *   
 * parameter
 * 1. fki(IN) : 설정할 KSN과 Future Key Set의 구조체 
 *      
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Import_Future_Key_Info(DukptFutureKeyInfo *fki);
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Import_Future_Key_Info_Ek(unsigned char *external_key, DukptFutureKeyInfo *fki);

/* DAMO_DUKPT_Power_On_Reset
 * description : 재부팅시 Future Key Set을 통해 Encryption Counter를 Reset시키는 함수
 */
void DAMO_DUKPT_Power_On_Reset();

/* DAMO_DUKPT_AES_Encrypt
 * description : DUKPT AES 암호화 함수(fki import --> DUKPT TK 생성 --> AES CBC 128bit enc --> fki export)
 *   
 * parameter
 * 1. enc(OUT) : 암호문 데이터를 저장할 포인터
 * 2. enc_len(OUT) : 암호문 데이터 길이를 저장할 포인터
 * 3. KSN(OUT) : KSN 데이터를 저장할 포인터
 * 4. KSN_len(OUT) : KSN 데이터 길이를 저장할 포인터
 * 5. org(IN) : 평문 데이터
 * 6. org_len(IN) : 평문 데이터 길이
 * 7. fki(IN/OUT) : 설정할 KSN과 Future Key Set의 구조체
 *            
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_AES_Encrypt(unsigned char *enc, size_t *enc_len, unsigned char *KSN, size_t *KSN_len, const unsigned char *org, size_t org_len, DukptFutureKeyInfo *fki);

/* DAMO_DUKPT_Client_PK_Auth_Start
 * description : 서버에서 랜덤 데이터와 개인키로 서명된 데이터를 입력받아 검증한 후 IPEK를 암호화하기 위한 키를 유도하기 위한 키를 공개키로 암호화하는 함수
 *   
 * parameter
 * 1. publicKeyFilePath(IN) : 공개키 파일 위치, pem 형식을 입력받음. 예) /home/user/public.pem 
 * 2. SrandData(IN)         : 서버쪽에서 받은 랜덤한 데이터, HEX
 * 3. HashData(IN)          : 서버쪽에서 받은 랜덤한 데이터의 SHA256 해쉬 데이터, HEX
 * 4. SignValue(IN)         : 서버쪽에서 받은 랜덤한 데이터의 개인키 서명 데이터, HEX
 * 5. OutRandValue(OUT)     : IPEK를 암호화하기 위한 키를 유도하기 위한 랜덤 키를 저장할 포인터, RAW
 * 6. OutEncRandValue(OUT)  : IPEK를 암호화하기 위한 키를 유도하기 위한 랜덤 키를 공개키로 암호화하여 저장할 포인터, HEX
 *            
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Client_PK_Auth_Start(unsigned char *publicKeyFilePath, unsigned char *SrandData, unsigned char *HashData, unsigned char *SignValue, unsigned char OutRandValueByte[16], unsigned char *OutEncRandValue);

/* DAMO_DUKPT_Client_PK_Auth_End
 * description : 서버에서 받은 IPEK X9.TR32 변환 데이터(암호화 및 MAC )를 복호화하여 IPEK를 얻는 함수
 *   
 * parameter
 * 1. publicKeyFilePath(IN) : 공개키 파일 위치, pem 형식을 입력받음. 예) /home/user/public.pem 
 * 2. EncIpekValue(IN)      : 서버쪽에서 받은 IPEK X9 TR31 변환 데이터, HEX
 * 3. ksn(IN)               : 서버에 요청한 ksn 데이터, RAW
 * 4. RandValue(IN)         : DAMO_DUKPT_Client_Verify_Encrypt에서 생성한 IPEK를 암호화하기 위한 키를 유도하기 위한 랜덤 데이터, RAW
 * 5. IpekValue(OUT)        : EncIpekValue를 X9 TR31 복호화 및 MAC 검증하여 나온 IPEK를 저장할 포인터, RAW
 *            
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Client_PK_Auth_End(unsigned char *publicKeyFilePath, unsigned char *EncIpekValue, unsigned char *ksn, unsigned char randValueByte[16], unsigned char IpekValue[16]);

/* DAMO_DUKPT_Client_PK_Auth_End_Ex
 * description : 서버에서 받은 IPEK X9.TR32 변환 데이터(암호화 및 MAC )를 복호화하여 IPEK를 얻는 함수, 10byte 이산 KSN 입력 받을 수 있도록 수정
 *   
 * parameter
 * 1. publicKeyFilePath(IN) : 공개키 파일 위치, pem 형식을 입력받음. 예) /home/user/public.pem 
 * 2. EncIpekValue(IN)      : 서버쪽에서 받은 IPEK X9 TR31 변환 데이터, HEX
 * 3. ksn(IN)               : 서버에 요청한 ksn 데이터, RAW
 * 4. ksn_len(IN)           : 서버에 요청한 ksn 데이터의 길이
 * 5. RandValue(IN)         : DAMO_DUKPT_Client_Verify_Encrypt에서 생성한 IPEK를 암호화하기 위한 키를 유도하기 위한 랜덤 데이터, RAW
 * 6. IpekValue(OUT)        : EncIpekValue를 X9 TR31 복호화 및 MAC 검증하여 나온 IPEK를 저장할 포인터, RAW
 *            
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Client_PK_Auth_End_Ex(unsigned char *publicKeyFilePath, unsigned char *EncIpekValue, unsigned char *ksn, size_t ksn_len, unsigned char randValueByte[16], unsigned char IpekValue[16]);

/* DAMO_DUKPT_Make_KSN10
 * description : TID가 7byte+3bit보다 클 경우에 KSN을 10byte로 변환해주는 함수
 *   
 * parameter
 * 1. ksn(IN)               : 입력받은 ksn 데이터 ( 뒤의 3byte는 CounterBit이며, 3byte를 제외한 값이 TID )
 * 2. ksn_len(IN)           : 입력받은 ksn 길이
 * 3. ksn10(OUT)            : ksn 데이터의 3byte제외한 값을 7byte로 변환하고 CounterBit 3byte를 추가한 10byte KSN
 * 4. ksn10MaxLen           : 결과를 받을 ksn10의 최대 버퍼 크기
 *            
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_DUKPT_Make_KSN10(const unsigned char *ksn, size_t ksn_len, unsigned char *ksn10 /* OUT */, size_t ksn10MaxLen);

/* DAMO_DUKPT_Client_Make_MacData
 * description : 암호화 데이터 검증을 위해 입력받은 암호화 데이터로 hmac 값을 만들어서 입력받은 암호화 데이터와 HMAC 데이터를 조합해 검증할 데이터를 만드는 함수
 * Author      : Jang Insoo
 * Date        : 2015.11.17
 *   
 * parameter
 * 1. dataType          : 어느 Client인지 식별
 * 2. st_enc            : 인증할 데이터를 만들 암호화 데이터, KSN, req_mac_key 등과 같은 정보
 * 3. outData           : HMAC 데이터와 암호화 데이터를 조합한 정보
 * 4. outDataLen        : HMAC 데이터와 암호화 데이터를 조합한 정보의 길이
 *
 * MAC Data             : KSN||암호문길이||암호문
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Client_Make_MacData(int dataType, void * st_enc, unsigned char* outData, int *outDataLen );

/*
*/
LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_KICC_POS_CLIENT(char* keyPath, char* in, int inLen, char* out, int* outLen);

#ifdef __cplusplus
}
#endif

#endif // __DUKPT_CLI_H__