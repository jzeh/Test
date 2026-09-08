#define AP_TS_SIZE 25
#define AP_PSK_SIZE 16
#define AP_TID_SIZE 7
#define AP_RAND_SIZE 16
#define AP_KEY_SIZE 16
#define AP_PSK_TID_SIZE (AP_PSK_SIZE+AP_TID_SIZE)
#define AP_PSK_TID_RAND_SIZE (AP_PSK_SIZE+AP_TID_SIZE+AP_RAND_SIZE)
#define AP_TID_RAND_SIZE (AP_TID_SIZE+AP_RAND_SIZE)
#define AP_KEY_RAND_SIZE (AP_KEY_SIZE+AP_RAND_SIZE)
#define AP_CHAL_SIZE 55
#define AP_RESP_SIZE 48
#define AP_SID_SIZE 16
#define AP_SID_TID_SIZE (AP_SID_SIZE+AP_TID_SIZE)
#define AP_SID_TID_RAND_SIZE (AP_SID_SIZE+AP_TID_SIZE+AP_RAND_SIZE)

/* Return Value */
#define DAMO_DUKPT_SUCCESS                        000000 /* Success */
#define DAMO_DUKPT_ERR_AP_GTK_NULL_POINTER       -100000 /* 임시키 생성시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_GTK_INVALID_LEN        -100001 /* 임시키 생성시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_GTK_GET_TKEY_FAIL      -100002 /* 임시키 생성 실패 */
#define DAMO_DUKPT_ERR_AP_MC_NULL_POINTER        -100003 /* Challenge 생성시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_MC_INVALID_LEN         -100004 /* Challenge 생성시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VC_NULL_POINTER        -100005 /* Challenge 검증시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_VC_INVALID_LEN         -100006 /* Challenge 검증시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VC_VERI_FAIL           -100007 /* Challenge 검증 실패 */
#define DAMO_DUKPT_ERR_AP_MR_NULL_POINTER        -100008 /* Response 생성시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_MR_INVALID_LEN         -100009 /* Response 생성시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VR_NULL_POINTER        -100010 /* Response 검증시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_VR_INVALID_LEN         -100011 /* Response 검증시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VR_VERI_FAIL           -100012 /* Response 검증 실패 */
#define DAMO_DUKPT_ERR_AP_MTCS_NULL_POINTER       -100103 /* TokenCS 생성시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_MTCS_INVALID_LEN        -100104 /* TokenCS 생성시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VTCS_NULL_POINTER       -100105 /* TokenCS 검증시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_VTCS_INVALID_LEN        -100106 /* TokenCS 검증시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VTCS_VERI_FAIL          -100107 /* TokenCS 검증 실패 */
#define DAMO_DUKPT_ERR_AP_MTSC_NULL_POINTER       -100108 /* TokenSC 생성시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_MTSC_INVALID_LEN        -100109 /* TokenSC 생성시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VTSC_NULL_POINTER       -100110 /* TokenSC 검증시 NULL 포인터 에러 */
#define DAMO_DUKPT_ERR_AP_VTSC_INVALID_LEN        -100111 /* TokenSC 검증시 유효하지 않는 길이 */
#define DAMO_DUKPT_ERR_AP_VTSC_VERI_FAIL          -100112 /* TokenSC 검증 실패 */

#if defined(EXPORT_DUKPTSERVER)
#define LIBSPEC_DUKPTSERVER  __declspec(dllexport)
#elif defined(IMPORT_DUKPTSERVER)
#define LIBSPEC_DUKPTSERVER  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTSERVER
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* DAMO_DUKPT_Get_Temp_Key
 * description : 임시키 생성 함수
 *
 * parameter
 * 1. tid(IN) : KSN 상위 7바이트
 * 2. tid_len(IN) : tid버퍼 길이
 * 3. c_random(OUT) : 클라이언트가 생성하는 16바이트 랜덤데이터
 * 4. c_random_len(IN) : c_random버퍼 길이
 * 5. key1(OUT) : Challenge데이터를 생성하는데 사용될 16바이트 임시키 데이터
 * 6. key1_len(IN) : key1버퍼 길이
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_DUKPT_Get_Temp_Key(const unsigned char *tid, size_t tid_len,
		unsigned char *c_random, size_t c_random_len, unsigned char *key1, size_t key1_len);

/* DAMO_DUKPT_Make_Challenge
 * description : 클라이언트가 서버에 전송할 Challenge데이터를 구성하는 함수
 *
 * parameter
 * 1. tid(IN) : KSN 상위 7바이트
 * 2. tid_len(IN) : tid버퍼 길이
 * 3. c_random(IN) : 클라이언트가 생성한 16바이트 랜덤데이터
 * 4. c_random_len(IN) : c_random버퍼 길이
 * 5. key1(IN) : Challenge데이터를 생성하는데 사용되는 16바이트 임시키 데이터
 * 6. key1_len(IN) : key1버퍼 길이
 * 4. send_chal(OUT) : 서버에 전송할 55바이트 Challenge데이터 => [7바이트 TID]+[16바이트 CRandom]+[32바이트 KEY1_HMAC(TID+CRandom)]
 * 5. send_chal_len(IN) : send_chal버퍼 길이
 *
 * return value
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_DUKPT_Make_Challenge(const unsigned char *tid, size_t tid_len, const unsigned char *c_random, size_t c_random_len,
		const unsigned char *key1, size_t key1_len, unsigned char *send_chal, size_t send_chal_len);

/* DAMO_DUKPT_Verify_Challenge
 * description : 서버에서 클라이언트가 전송한 Challenge데이터를 검증하는 함수
 *
 * parameter
 * 1. recv_chal(IN) : 서버가 클라이언트로부터  수신한 55바이트 Challenge데이터 => [7바이트 TID]+[16바이트 CRandom]+[32바이트 KEY1_HMAC(TID+CRandom)]
 * 2. recv_chal_len(IN) : recv_chal버퍼 길이
 * 3. tid(OUT) : Challenge데이터로부터 추출된 상위 7바이트 TID
 * 4. tid_len(IN) : tid버퍼 길이
 * 5. c_random(OUT) : Challenge데이터로부터 추출된 16바이트 클라이언트 랜덤데이터
 * 6. c_random_len(IN) : c_random버퍼 길이
 * 7. key1(OUT) : Response데이터 암호화 키를 생성하는데 사용되는 16바이트 임시키 데이터
 * 8. key1_len(IN) : key1버퍼 길이
 *
 * return value
 *  Challenge 데이터 검증 성공시 , ret = 0
 *  실패시, ret < 0
 */
int DAMO_DUKPT_Verify_Challenge(const unsigned char *recv_chal, size_t recv_chal_len, unsigned char *tid, size_t tid_len,
		unsigned char *c_random, size_t c_random_len, unsigned char *key1, size_t key1_len);

/* DAMO_DUKPT_Make_Response
 * description : 서버가 클라이언트에 전송할 Response데이터를 구성하는 함수
 *
 * parameter
 * 1. ipek(IN) : 클라이언트에 전송할 16바이트 ipek(초기 PIN 암호화키)
 * 2. ipek_len(IN) : ipek버퍼 길이
 * 3. c_random(IN) : Challenge데이터로부터 추출된 16바이트 클라이언트 랜덤데이터
 * 4. c_random_len(IN) : c_random버퍼 길이
 * 5. key1(IN) : Response데이터 암호화 키를 생성하는데 사용되는 16바이트 임시키 데이터
 * 6. key1_len(IN) : key1버퍼 길이
 * 7. send_resp(OUT) : 클라이언트에 전송할 48바이트 Response데이터 => [16바이트 SRandom]+[32바이트 KEY2_ENC(IPEK+CRandom)]
 * 8. send_resp_len(IN) : send_resp버퍼 길이 
 *
 *  if successful, ret = 0
 *  otherwise, ret < 0
 */
int DAMO_DUKPT_Make_Response(const unsigned char *ipek, size_t ipek_len, const unsigned char *c_random, size_t c_random_len,
		const unsigned char *key1, size_t key1_len, unsigned char *send_resp, size_t send_resp_len);

/* DAMO_DUKPT_Verifiy_Response
 * description : 클라이언트에서 서버가 전송한 Response데이터를 검증하는 함수
 *
 * parameter
 * 1. recv_resp(IN) : 클라이언트가 서버로부터 수신한 48바이트 Response데이터 => [16바이트 SRandom]+[32바이트 KEY2_ENC(IPEK+CRandom)]
 * 2. recv_resp_len(IN) : recv_resp버퍼 길이 
 * 3. c_random(IN) : 클라이언트가 생성한 16바이트 랜덤데이터 
 * 4. c_random_len(IN) : c_random버퍼 길이
 * 5. key1(IN) : Response데이터 복호화 키를 생성하는데 사용되는 16바이트 임시키 데이터
 * 6. key1_len(IN) : key1버퍼 길이
 * 7. ipek(OUT) : 클라이언트가 사용할 ipek(초기 PIN 암호화키)
 * 8. ipek_len(IN) : ipek버퍼 길이
 *
 * return value
 *  Response 데이터 검증 성공시 , ret = 0
 *  실패시, ret < 0
 */
int DAMO_DUKPT_Verify_Response(const unsigned char *recv_resp, size_t recv_resp_len, const unsigned char *c_random, size_t c_random_len,
		const unsigned char *key1, size_t key1_len, unsigned char *ipek, size_t ipek_len);

/* DAMO_DUKPT_Make_TokenCS
 * description : 클라이언트가 서버에 전송하는 Timestamp기반 인증 토큰을 생성하는 함수
 *
 * parameter
 * 1. svr_info(IN) : 서버 식별자 정보 
 * 2. svr_info_len(IN) : 서버 식별자 정보 길이
 * 3. tid(IN) : TID(클라이언트 식별자)
 * 4. tid_len(IN) : TID 길이
 * 5. c_random(IN) : 클라이언트가 생성한 16바이트 랜덤데이터
 * 6. c_random_len(IN) : c_random버퍼 길이
 * 7. key1(IN) : 인증토큰을 생성하는데 사용되는 키
 * 8. key1_len(IN) : key1 길이
 * 9. send_token_cs(OUT) : 전송할 토큰 데이터 
 * 10. send_token_cs_len(INOUT) : send_token_cs버퍼 길이(IN), 토큰 데이터 길이(OUT)
 *
 * return value
 *  Response 데이터 검증 성공시 , ret = 0
 *  실패시, ret < 0
 */
int DAMO_DUKPT_Make_TokenCS(const unsigned char *svr_info, size_t svr_info_len, const unsigned char *tid, size_t tid_len,
		const unsigned char *c_random, size_t c_random_len, const unsigned char *key1, size_t key1_len,
		unsigned char *send_token_cs, size_t *send_token_cs_len);

/* DAMO_DUKPT_Verify_TokenCS
 * description : 서버에서 클라이언트가 전송하는 Timestamp기반 인증 토큰을 검증하는 함수
 *
 * parameter
 * 1. ts_interval(IN) : 인증 토큰을 검증시 Timestamp의 허용 시간 범위(단위: 초) 
 * 2. recv_token_cs(IN) : 서버가 수신한 Timestamp기반 인증 토큰 
 * 3. recv_token_cs_len(IN) : 서버가 수신한 Timestamp기반 인증 토큰 길이
 * 4. svr_info(IN) : 서버 식별자 정보
 * 5. svr_info_len(IN) : 서버 식별자 정보 길이
 * 6. tid(OUT) : 인증 토큰으로부터 추출된 상위 7바이트 TID 
 * 7. tid_len(IN) : tid버퍼 길이
 * 8. key1(OUT) : 인증 토큰을 생성하는데 사용되는 키
 * 9. key1_len(IN) : key1버퍼 길이
 *
 * return value
 *  Response 데이터 검증 성공시 , ret = 0
 *  실패시, ret < 0
 */
int DAMO_DUKPT_Verify_TokenCS(int ts_interval, const unsigned char *recv_token_cs, size_t recv_token_cs_len,
		const unsigned char *svr_info, size_t svr_info_len, unsigned char *tid, size_t tid_len, unsigned char *key1, size_t key1_len);

LIBSPEC_DUKPTSERVER
int DAMO_DUKPT_Server_Verify_TokenCS (unsigned char *sid, size_t sid_len, int ts_interval, const unsigned char *recv_token_cs, size_t recv_token_cs_len,
		 const unsigned char *svr_info, size_t svr_info_len, unsigned char *tid, size_t tid_len, unsigned char *key1, size_t key1_len);

/* DAMO_DUKPT_Make_TokenSC 
 * description : 서버가 클라이언트에 전송하는 Timestamp기반 인증 토큰을 생성하는 함수
 *
 * parameter
 * 1. tid(IN) : TID(클라이언트 식별자)
 * 2. tid_len(IN) : TID 길이
 * 3. ipek(IN) : 클라이언트가 사용할 ipek(초기 PIN 암호화키)
 * 4. ipek_len(IN) : ipek버퍼 길이
 * 5. key1(IN) : 인증 토큰을 생성하는데 사용되는 키
 * 6. key1_len(IN) : key1 길이
 * 7. send_token_sc(OUT) : 전송할 토큰 데이터 
 * 8. send_token_sc_len(INOUT) : send_token_sc버퍼 길이(IN), 토큰 데이터 길이(OUT)
 *
 * return value
 *  Response 데이터 검증 성공시 , ret = 0
 *  실패시, ret < 0
 */
int DAMO_DUKPT_Make_TokenSC(const unsigned char *tid, size_t tid_len,
		const unsigned char *ipek, size_t ipek_len, const unsigned char *key1, size_t key1_len,
		unsigned char *send_token_sc, size_t *send_token_sc_len);

/* DAMO_DUKPT_Verify_TokenSC
 * description : 클라이언트에서 서버가 전송하는 Timestamp기반 인증 토큰을 검증하는 함수
 *
 * parameter
 * 1. ts_interval(IN) : 인증 토큰을 검증시 Timestamp의 허용 시간 범위(단위: 초) 
 * 2. recv_token_sc(IN) : 클라이언트가 수신한 Timestamp기반 인증 토큰 
 * 3. recv_token_sc_len(IN) : 클라이언트가 수신한 Timestamp기반 인증 토큰 길이
 * 4. tid(IN) : TID(클라이언트 식별자)  
 * 5. tid_len(IN) : TID 길이
 * 6. key1(IN) : 인증 토큰을 검증하는데 사용되는 키
 * 7. key1_len(IN) : key1 길이
 * 8. ipek(OUT) : 클라이언트가 사용할 ipek(초기 PIN 암호화키)
 * 9. ipek_len(IN) : ipek버퍼 길이
 *
 * return value
 *  Response 데이터 검증 성공시 , ret = 0
 *  실패시, ret < 0
 */
int DAMO_DUKPT_Verify_TokenSC(int ts_interval, const unsigned char *recv_token_sc, size_t recv_token_sc_len,
		const unsigned char *tid, size_t tid_len, const unsigned char *key1, size_t key1_len, unsigned char *ipek, size_t ipek_len);

#ifdef __cplusplus
}
#endif

