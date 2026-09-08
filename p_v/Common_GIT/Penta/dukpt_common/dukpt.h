#ifndef __DUKPT_H__
#define __DUKPT_H__
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char ksn[10]; /* key serial number(encryption counter 포함) */
	unsigned char enc_pin_block[8]; /* encrypted pin block */
	unsigned char pin_enc_key[16]; /* pin enc key */
	unsigned char req_mac_key[16]; /* request mac key */
	unsigned char res_mac_key[16]; /* response mac key */
	unsigned char req_enc_key[16]; /* request enc key */
	unsigned char res_enc_key[16]; /* response enc key */
} DukptPinEntry;

typedef struct {
	unsigned char key_serial_number[17];
	unsigned char future_key_register[21][45];
} DukptFutureKeyInfo;

typedef struct {
	unsigned char key_serial_number[17];
	unsigned char future_key_register[21][25];
} DukptFutureKeyInfoOld;

typedef struct {
  unsigned char *encData; /*encData의 길이를 미리 예측할 수 없어서 포인터 변수로 선언*/
  long encDataLen;
  unsigned char ksn[10];
  int ksnLen;
  unsigned char req_mac_key[16];
} DukptClientMacData1;

enum DAMO_DUKPT_PADDING {
	DAMO_PKCS7_PADDING=0,
	DAMO_ZERO_PADDING,
};
enum DAMO_DUKPT_MSB {
	DAMO_TR31_NOT_STD=0,
	DAMO_TR31_STD,
};
enum DAMO_DUKPT_CLIENT_WORD {
	DAMO_DUKPT_WORD_HEX=0,
	DAMO_DUKPT_WORD_STRING,
};

enum KICC_POS_RETURN {
	DAMO_KICC_POS_SUCCESS=0,
	DAMO_KICC_INPUT_ERROR,
	DAMO_KICC_CLIENT_FRIST_IN_ERROR,
	DAMO_KICC_CLIENT_REQUEST_ERROR,
	DAMO_KICC_SERVER_ANSWER_ERROR,
};

/* 암호화 값 MAC 체크를 할 때 어느 Client에서 들어왔는지 식별 */
enum DAMO_DUKPT_MACDATA {
  DAMO_DUKPT_CLIENT_MACDATA1 = 0, /* GS Retail */
};

/* 필요한 키에 대한 플래그(DAMO_DUKPT_Request_Pin_Entry함수에서 사용) */
#define DAMO_DUKPT_FLAG_USE_NOT_KEY		  0x0000 /* 모든 키를 사용하지 않음 */
#define DAMO_DUKPT_FLAG_USE_PIN_ENC_KEY	  0x0001 /* PIN 암호화키 사용 */
#define DAMO_DUKPT_FLAG_USE_DATA_MAC_KEY  0x0002 /* DATA MAC키 사용 */
#define DAMO_DUKPT_FLAG_USE_DATA_ENC_KEY  0x0004 /* DATA 암호화키 사용 */
#define DAMO_DUKPT_FLAG_USE_ALL_KEY		  0x0007 /* 모든 키 사용 */
#endif //__DUKPT_H__