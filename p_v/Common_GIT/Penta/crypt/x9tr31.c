#include <stdio.h>
#include <string.h>
#include "x9tr31.h"
#include "des.h"

#ifdef DUKPT_SERVER
  #include "dukpt_server_module.h"
#else
  #include "dukpt.h"
  #include "dukpt_cli_config.h"
#endif

LIBSPEC_DUKPTCRYPT
void DAMO_DUKPT_ByteToHex( char *str, int *strLen, 
                unsigned char *in, int inLen )
{
  int i, i2;
  char cpAscii[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  *strLen = inLen * 2;

  for( i=0; i<inLen; i++ )
  {
    i2 = i * 2;
    str[i2] = cpAscii[(in[i]&0xF0)>>4];
    str[i2 + 1] = cpAscii[(in[i]&0x0F)];    
  }
  str[*strLen]='\0';
}

LIBSPEC_DUKPTCRYPT
int DAMO_DUKPT_HexToByte(unsigned char *out, int *outLen, 
               char *str, int strLen)
{
  int i, i2 = 0;
  *outLen = strLen >> 1;
  for(i=0; i<*outLen; i++)
  {    
    out[i]=0;
    if(str[i2] >= '0' && str[i2] <= '9')
      out[i] = (str[i2] - 48) << 4;
    else if(str[i2] >= 'A' && str[i2] <= 'F')
      out[i] = (str[i2] - 55) << 4;
    else
      return -1;
    if(str[i2 + 1] >= '0' && str[i2 + 1] <= '9')
      out[i] += str[i2 + 1] - 48;
    else if(str[i2 + 1] >= 'A' && str[i2 + 1] <= 'F')
      out[i] += str[i2 + 1] - 55;
    else
    {
      *outLen = 0;
      return -1;
    }
    i2 += 2;
  }
  return 0;
}

void DAMO_X9TR31_XOR(unsigned char *in, int dataSize, unsigned char *out)
{
  int i;
  for(i = 0; i < dataSize; i++)
  {
    out[i] ^= in[i];
  }
}

void DAMO_X9TR31_leftBitShift(unsigned char *in, int dataSize, unsigned char *out)
{
  int i=0;

  memset(out, 0x00, dataSize);

  if ( dataSize>0)
    out[0] = in[0] << 1;

  for ( i=1; i<dataSize; i++ )
  {
    if ( in[i] & 0x80 ) 
      out[i-1] += 1;
    out[i] = in[i] << 1;
  }
}

void DAMO_X9TR31_GetSubKey(unsigned char *keyBlockProtectionKey, unsigned char *K1, unsigned char *K2, int compatibility)
{
  unsigned char R64[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B };

  unsigned char S[8] = {0};

  unsigned char inputData[8] = {0 };
  DES3_CTX ctx;

  DAMO_CRYPT_DES3_Set_EKey2( &ctx, keyBlockProtectionKey );
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, inputData, inputData, sizeof(inputData), S );    

  DAMO_X9TR31_leftBitShift(S, 8, K1);

  if ( (S[0] & 0x80) || compatibility )
    DAMO_X9TR31_XOR(R64, 8, K1);

  DAMO_X9TR31_leftBitShift(K1, 8, K2);

  if ( (K1[0] & 0x80) || compatibility )
    DAMO_X9TR31_XOR(R64, 8, K2);
}

int DAMO_CRYPT_AES_Encrypt_Core(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, size_t pad);

void DAMO_X9TR31_AES_GetSubKey(unsigned char *key, unsigned char *K1, unsigned char *K2, int compatibility)
{
  unsigned char R128[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87 };

  unsigned char S[16] = {0};
  size_t        SLen;

  unsigned char inputData[16] = {0};


  DAMO_CRYPT_AES_Encrypt_Core(S, &SLen, inputData, sizeof(inputData), key, 16, AES_128, CBC_MODE, inputData, DAMO_ZERO_PADDING);

  DAMO_X9TR31_leftBitShift(S, 16, K1);

  if ( (S[0] & 0x80) || compatibility )
    DAMO_X9TR31_XOR(R128, 16, K1);

  DAMO_X9TR31_leftBitShift(K1, 16, K2);

  if ( (K1[0] & 0x80) || compatibility )
    DAMO_X9TR31_XOR(R128, 16, K2);
}

void DAMO_X9TR31_GetKeyBlockEncryptionKey(unsigned char *keyBlockProtectionKey, unsigned char *subKey, unsigned char *encryptionKey)
{
  unsigned char inputData1[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
  unsigned char inputData2[8] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
  DES3_CTX ctx;

  DAMO_CRYPT_DES3_Set_EKey2( &ctx, keyBlockProtectionKey );
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, subKey, inputData1, sizeof(inputData1), encryptionKey );    
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, subKey, inputData2, sizeof(inputData2), &encryptionKey[8] );    
}

void DAMO_X9TR31_GetKeyBlockMACKey(unsigned char *keyBlockProtectionKey, unsigned char *subKey, unsigned char *MACKey)
{
  unsigned char inputData1[8] = {0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80};
  unsigned char inputData2[8] = {0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80};
  DES3_CTX ctx;

  DAMO_CRYPT_DES3_Set_EKey2( &ctx, keyBlockProtectionKey );
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, subKey, inputData1, sizeof(inputData1), MACKey );    
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, subKey, inputData2, sizeof(inputData2), &MACKey[8] );    
}

void DAMO_X9TR31_Get_Enc_Mac_Key(unsigned char *keyBlockProtectionKey, unsigned char *encryptionKey, unsigned char *MACKey)
{
  unsigned char K1[8];
  unsigned char K2[8];

#ifdef DUKPT_SERVER
  if( initValues.msbCheck == DAMO_TR31_STD )
  {
    DAMO_X9TR31_GetSubKey(keyBlockProtectionKey, K1, K2, 0);
  }
  else
  {
    DAMO_X9TR31_GetSubKey(keyBlockProtectionKey, K1, K2, 1);
  }
#elif defined(DUKPT_CLIENT_TR31_STD)
  DAMO_X9TR31_GetSubKey(keyBlockProtectionKey, K1, K2, 0);
#else
  DAMO_X9TR31_GetSubKey(keyBlockProtectionKey, K1, K2, 1);
#endif

  DAMO_X9TR31_GetKeyBlockEncryptionKey(keyBlockProtectionKey, K1, encryptionKey);
  DAMO_X9TR31_GetKeyBlockMACKey(keyBlockProtectionKey, K1, MACKey);
}


//TEST

void DAMO_X9TR31_MakeMac(unsigned char *Header, unsigned char *BinaryKeyData, int BinaryKeyDataLen, unsigned char *macKey, unsigned char *K1, unsigned char *mac)
{
  unsigned char l_S[8] = {0};
  unsigned char l_iv[8] = {0x00 };
  unsigned char l_HeaderData[16];
  unsigned char l_BinaryKeyData[50];
  DES3_CTX ctx;

  memcpy(l_HeaderData, Header, 16);
  memcpy(l_BinaryKeyData, BinaryKeyData, BinaryKeyDataLen);

  DAMO_CRYPT_DES3_Set_EKey2( &ctx, macKey );
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, l_iv, l_HeaderData, 8, l_S );

  DAMO_X9TR31_XOR(&l_HeaderData[8], 8, l_S);

  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, l_iv, l_S, sizeof(l_S), l_S );

  //// 이진 키 데이터

  DAMO_X9TR31_XOR(l_BinaryKeyData, 8, l_S);

  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, l_iv, l_S, sizeof(l_S), l_S );

  
  DAMO_X9TR31_XOR(&l_BinaryKeyData[8], 8, l_S);
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, l_iv, l_S, sizeof(l_S), l_S );

  DAMO_X9TR31_XOR(&l_BinaryKeyData[16], 8, K1);

  DAMO_X9TR31_XOR(K1, 8, l_S);
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, l_iv, l_S, sizeof(l_S), mac ); //mac 생성
}

void DAMO_X9TR31_MakeCipher(unsigned char *BinaryKeyData, unsigned int BinaryKeyDataLen, unsigned char *keyBlockProtectionKey, unsigned char *mac, unsigned char *K1, unsigned char *msg)
{
  unsigned char l_tmpMsg[32];
  int msgLen = 0;
  DES3_CTX ctx;

  DAMO_CRYPT_DES3_Set_EKey2( &ctx, keyBlockProtectionKey );
  DAMO_CRYPT_DES3_CBC( &ctx, DAMO_CRYPT_DES_ENC, mac, BinaryKeyData, BinaryKeyDataLen, l_tmpMsg );

  // buf
  DAMO_DUKPT_ByteToHex((char*)msg, &msgLen, l_tmpMsg, BinaryKeyDataLen);
  
}

int DAMO_DUKPT_X9TR31_CreateMsg(unsigned char *keyType, unsigned char *KeyBlockProtectionKey, int KeyBlockProtectionKeyLen, unsigned char *BinaryKeyData, int BinaryKeyDataLen, int padType, unsigned char *msg, int *msgLen)
{
  unsigned char km1[8];
  unsigned char km2[8];

  unsigned char encKey[16];
  unsigned char macKey[16];

  unsigned char Header[16];
  unsigned char mac[8];
  //int macLen = 0;

  unsigned char rm = 0;

  unsigned char TempBinaryKeyData[50];
  int TempBinaryKeyDataLen = 0;
  char BinaryKeyDataLenToHex[5];

  sprintf(BinaryKeyDataLenToHex, "%04x", BinaryKeyDataLen * 8);
  DAMO_DUKPT_HexToByte(TempBinaryKeyData, &TempBinaryKeyDataLen, BinaryKeyDataLenToHex, 4);

  if(strcmp((char const*)keyType, "ipek") == 0) //IPEK
  {
      //0x42, 0x31 //IPEK
		  //Defined mode of use values
		  //0x42

	  unsigned char tmpHeader[16] = {0x42, 0x30, 0x30, 0x38, 0x30, 0x42, 0x31, 0x54, 0x42, 0x30, 0x30,
			0x45, 0x30, 0x30, 0x30, 0x30};
	  memcpy(Header, tmpHeader, 16);
	//Header : 16, KeyLen : 2, Key : 16, Pad : 6, Mac : 8 = 48
  }
  else if(strcmp((char const*)keyType, "sample") == 0) //sample
  {
	  unsigned char tmpHeader[16] = {0x42, 0x30, 0x30, 0x38, 0x30, 0x50, 0x30, 0x54, 0x45, 0x30, 0x30,
			0x45, 0x30, 0x30, 0x30, 0x30};
	  memcpy(Header, tmpHeader, 16);
  }
  else //error
  {
	  return DAMO_X9TR31_CREATE_ERROR_KEYTYPE;
  }

  DAMO_X9TR31_Get_Enc_Mac_Key(KeyBlockProtectionKey, encKey, macKey); //encKey, macKey 만드는 연산

  /////////////////// Start

  if( padType == 123)
  {
	  char pad[6] = {0x1C, 0x29, 0x65, 0x47, 0x3C, 0xE2};
	  memcpy(TempBinaryKeyData+2, BinaryKeyData, BinaryKeyDataLen);
	  memcpy(TempBinaryKeyData + 2 + BinaryKeyDataLen, pad, 6);
	  rm = 6;
  }
  else if ( padType == DAMO_PKCS7_PADDING )
  {
	  rm = (unsigned char)(8 - ( (BinaryKeyDataLen+2) % 8));
  }
  else if( padType == DAMO_ZERO_PADDING )
  {
	  if ( (BinaryKeyDataLen+2) % 8 == 0)
	  {
		  rm = 0;
	  }
	  else
	  {
		  rm = (unsigned char)(8 - ( (BinaryKeyDataLen+2) % 8));
	  }
  }
  else
  {
	  return DAMO_CRYPT_ERR_TDES_ENC_INVALID_PAD;
  }

  memcpy(TempBinaryKeyData+2, BinaryKeyData, BinaryKeyDataLen);

  if( padType == DAMO_PKCS7_PADDING )
  {
	//servier pkcs7 padding
    memset(TempBinaryKeyData + 2 + BinaryKeyDataLen, rm, rm);
  }
  else if( padType != 123)
  {
	//server zero padding
  	memset(TempBinaryKeyData + 2 + BinaryKeyDataLen, 0x00, rm);
  }


#ifdef DUKPT_SERVER
  if( initValues.msbCheck == DAMO_TR31_STD )
  {
    DAMO_X9TR31_GetSubKey(macKey, km1, km2, 1);
  }
  else
  {
    DAMO_X9TR31_GetSubKey(macKey, km1, km2, 0);
  }
#elif defined(DUKPT_CLIENT_TR31_STD)
	DAMO_X9TR31_GetSubKey(macKey, km1, km2, 1);
#else
	DAMO_X9TR31_GetSubKey(macKey, km1, km2, 0);  
#endif

  memcpy(msg, Header, 16);
  TempBinaryKeyDataLen = 2 + BinaryKeyDataLen + rm;
  DAMO_X9TR31_MakeMac(Header, TempBinaryKeyData, TempBinaryKeyDataLen, macKey, km1, mac); //mac 만드는 연산
  DAMO_X9TR31_MakeCipher(TempBinaryKeyData, TempBinaryKeyDataLen, encKey, mac, km1, &msg[16]); //Cipher 만드는 연산 -> msg에 통합
  DAMO_DUKPT_ByteToHex((char*)&msg[16+( (2+BinaryKeyDataLen+rm)*2)], msgLen, mac, 8);
  *msgLen = 16+( (2+BinaryKeyDataLen+rm)*2)+(8*2);

  return 0;
}

void DAMO_X9TR31_BinaryKeyDecrypt(unsigned char *Ciphertext, int CiphertextLen, unsigned char *encKey, unsigned char *mac, unsigned char *BinaryKeyData, int *TmpBinaryKeyDataLen)
{
  DES3_CTX ctx;

  DAMO_CRYPT_DES3_Set_DKey2( &ctx, encKey );
  
  DAMO_CRYPT_DES3_CBC( &ctx, 0, mac, Ciphertext, CiphertextLen, BinaryKeyData );
  *TmpBinaryKeyDataLen = CiphertextLen;
}

int macCheck(unsigned char *orgMac, unsigned char *makeMac)
{
	int i;

    for(i=0; i<8; i++)
    {
	   if (orgMac[i] != makeMac[i])
	   {
		   return -1;
	   }
    }

	return 0;
}

int DAMO_DUKPT_X9TR31_InterpretMsg(unsigned char *KeyBlockProtectionKey, int KeyBlockProtectionKeyLen, unsigned char *msg, int msgLen, int padType, unsigned char *BinaryKeyData, int *BinaryKeyDataLen)
{
  unsigned char encKey[16];
  unsigned char macKey[16];
  unsigned char Header[16];
  unsigned char TmpBinaryKeyData[64];
  unsigned char l_BinaryKeyData[32];
  unsigned char orgMac[16];
  unsigned char mac[8];
  int macLen = 0;
  unsigned char km1[8];
  unsigned char km2[8];
  unsigned char rm = 0;
  unsigned int outLen = 0;
  int TmpBinaryKeyDataLen = 0;
  
  int CipherLen = msgLen-16-16; //msgLen - Header - Mac
  
  if(msgLen < 16)
  {
	  return DAMO_X9TR31_INTERPRETATION_ERROR_MSG_LEN;
  }
  
  memcpy(Header, msg, 16);

  DAMO_DUKPT_HexToByte(orgMac, &macLen, (char*)msg+16+CipherLen, 16);
  DAMO_DUKPT_HexToByte(TmpBinaryKeyData, &TmpBinaryKeyDataLen, (char*)msg+16, CipherLen);
	
  DAMO_X9TR31_Get_Enc_Mac_Key(KeyBlockProtectionKey, encKey, macKey); //encKey, macKey 만드는 연산
  
  DAMO_X9TR31_BinaryKeyDecrypt(TmpBinaryKeyData, TmpBinaryKeyDataLen, encKey, orgMac, l_BinaryKeyData, BinaryKeyDataLen); //복호화 -> IPEK 얻는 과정

#ifdef DUKPT_SERVER
  if( initValues.msbCheck == DAMO_TR31_STD )
  {
    DAMO_X9TR31_GetSubKey(macKey, km1, km2, 1);
  }
  else
  {
    DAMO_X9TR31_GetSubKey(macKey, km1, km2, 0);
  }
#elif defined(DUKPT_CLIENT_TR31_STD)
	DAMO_X9TR31_GetSubKey(macKey, km1, km2, 1);
#else
	DAMO_X9TR31_GetSubKey(macKey, km1, km2, 0);  
#endif

  DAMO_X9TR31_MakeMac(Header, l_BinaryKeyData, *BinaryKeyDataLen, macKey, km1, mac); //mac 만드는 연산

  if (macCheck(orgMac, mac) != 0)
  {
	  return DAMO_X9TR31_INTERPRETATION_ERROR_MAC_VERIFY; //mac 검증 실패
  }

  
  /*****************************************************************************************************************************************/
  // 복호화 -> Mac -> 패딩 : 패딩은 복호화쪽에서 해야 정상이지만, mac 만드는 연산에 패딩 값도 필요하기 때문에 패딩 처리를 뒤쪽에서 처리 함 //
  /*****************************************************************************************************************************************/
  
  //패딩 처리
  if (padType == 123) //123 sample
  {
	  rm = 6;
	  outLen = *BinaryKeyDataLen - rm;
  }
  else
  {
	  rm = l_BinaryKeyData[*BinaryKeyDataLen - 1];

	  if( padType == DAMO_ZERO_PADDING )
	  {
		  int i;
		  //servier zero padding
		  for(i = *BinaryKeyDataLen-1 ; i > 0 ; i--)
		  {
			  if(l_BinaryKeyData[i] != '\0')
				  break;
		  }
		  outLen = i + 1;

	  }
	  else if( padType == DAMO_PKCS7_PADDING )
	  {
		  //server pkcs7 padding
		  if ( rm == 0 || rm > 8 )
		  {
			  outLen = 0;
			  return DAMO_CRYPT_ERR_TDES_DEC_INVALID_PAD;
		  }

		  outLen = *BinaryKeyDataLen - rm;
	  }
	  else
	  {
		  return DAMO_CRYPT_ERR_TDES_DEC_INVALID_PAD;
	  }
  }
  
  *BinaryKeyDataLen = outLen-2;
  memcpy(BinaryKeyData, l_BinaryKeyData+2, *BinaryKeyDataLen);

  return 0;
}