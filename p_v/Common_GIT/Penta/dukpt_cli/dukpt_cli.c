#include <stdio.h>
#if WIN32
#include <winsock.h>
#else
//#include <arpa/inet.h>
#endif
#include "dukpt_cli.h"
#include "auth_proto.h"
#include "des.h"
#include "aes.h"
#include "sha256.h"
#include "base64_penta.h"
//#include "rng.h"
//#include "x9tr31.h"

//#include "damo_pkcrypt.h"
//#include "hmac_sha256.h"

static unsigned char key_serial_number[10];

static unsigned char shift_register_bit_num;

static unsigned int shift_register;

static unsigned char key_register[16];

static unsigned char future_key_register[21][17];

static unsigned char *current_key_pointer;

static unsigned char crypto_register1[8];

static unsigned char DAMO_DUKPT_Gen_LRC(const unsigned char *data, int data_len);

static void DAMO_DUKPT_Crypto_Register(const unsigned char key[16], const unsigned char in[8], unsigned char out[8]);

static void DAMO_DUKPT_Triple_DEA_Encrypt();

static void DAMO_DUKPT_Set_Bit();

static void DAMO_DUKPT_New_Key();

static void DAMO_DUKPT_New_Key1();

static void DAMO_DUKPT_New_Key2();

static void DAMO_DUKPT_New_Key3();

static void DAMO_DUKPT_New_Key4();

static void DAMO_DUKPT_Request_Pin_Entry1(int flags, DukptPinEntry *pEntry);

static void DAMO_DUKPT_Request_Pin_Entry2(int flags, DukptPinEntry *pEntry);

void DAMO_DUKPT_Gen_Non_Reversible_Key(unsigned char key[16], const unsigned char in[8], unsigned char out[16]);

void DAMO_DUKPT_Derive_PEK(unsigned char key[16], DukptPinEntry *pEntry);

void DAMO_DUKPT_Derive_MAK(unsigned char key[16], DukptPinEntry *pEntry);

void DAMO_DUKPT_Derive_DEK(unsigned char key[16], DukptPinEntry *pEntry);

extern int  DAMO_DUKPT_HexToByte(unsigned char *out, int *outLen, char *str, int strLen);
extern int DAMO_PKCRYPT_pk_verify( char* pubKeyFilePath, unsigned char* hash, size_t hashLen, unsigned char* sign, size_t signLen);
extern int DAMO_CRYPT_RNG( unsigned char *out, size_t out_len );
extern int DAMO_PKCRYPT_pk_encrypt( char* pubKeyFilePath, unsigned char* in, size_t inLen, unsigned char* out, size_t* outLen, int outMax);
extern void DAMO_DUKPT_ByteToHex(char *str, int *strLen, unsigned char *in, int inLen);
extern void DAMO_X9TR31_Get_Enc_Mac_Key(unsigned char *keyBlockProtectionKey, unsigned char *encryptionKey, unsigned char *MACKey);
extern int DAMO_CRYPT_HMAC_SHA256(const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len, unsigned char *digest);
extern int DAMO_PKCRYPT_pk_encrypt( char* pubKeyFilePath, unsigned char* in, size_t inLen, unsigned char* out, size_t* outLen, int outMax);
extern unsigned long htonl(unsigned long);

static unsigned char DAMO_DUKPT_Gen_LRC(const unsigned char *data, int data_len)
{
	int i;
	unsigned char LRC;

#if 1
	/* International standard ISO 1155 states that a longitudinal redundancy check for a sequence of bytes
	 * may be computed in software by the following algorithm */
	LRC = 0;
	for (i = 0; i < data_len; i++)
	{
		LRC = (LRC + data[i]) & 0xff;
	}
	LRC = (((LRC ^ 0xff) + 1) & 0xff);
#endif

#if 0
	LRC = 0;
	for (i = 0; i < data_len; i++)
	{
		LRC ^= data[i];
	}
#endif

	return LRC;
}

static void DAMO_DUKPT_Crypto_Register(const unsigned char key[16], const unsigned char in[8], unsigned char out[8])
{
	DES_CTX ctx;
	unsigned char tmp[8];

	tmp[0] = in[0]^key[8];
	tmp[1] = in[1]^key[9];
	tmp[2] = in[2]^key[10];
	tmp[3] = in[3]^key[11];
	tmp[4] = in[4]^key[12];
	tmp[5] = in[5]^key[13];
	tmp[6] = in[6]^key[14];
	tmp[7] = in[7]^key[15];

	DAMO_CRYPT_DES_Set_EKey( &ctx, key );
	DAMO_CRYPT_DES_Block( &ctx, tmp, tmp );

	out[0] = tmp[0]^key[8];
	out[1] = tmp[1]^key[9];
	out[2] = tmp[2]^key[10];
	out[3] = tmp[3]^key[11];
	out[4] = tmp[4]^key[12];
	out[5] = tmp[5]^key[13];
	out[6] = tmp[6]^key[14];
	out[7] = tmp[7]^key[15];
}

static void DAMO_DUKPT_Triple_DEA_Encrypt()
{
	DES3_CTX ctx;

	DAMO_CRYPT_DES3_Set_EKey2( &ctx, key_register );
	DAMO_CRYPT_DES3_Block( &ctx, crypto_register1, crypto_register1 );
}

static void DAMO_DUKPT_Set_Bit()
{
	unsigned int encryption_counter;
	
	shift_register_bit_num = 20;

	encryption_counter = ((key_serial_number[7]&0x1f)<<16)|(key_serial_number[8]<<8)|key_serial_number[9];

	if(!encryption_counter)
	{
		shift_register = 0;
		return;
	}

	shift_register_bit_num = 20;

	while(!(encryption_counter & 0x00000001))
	{
		encryption_counter = encryption_counter >> 1;
		shift_register_bit_num--;
	}
	shift_register = 1 << (20 - shift_register_bit_num);
}

static void DAMO_DUKPT_New_Key()
{
	unsigned int encryption_counter;
	unsigned int tmp;
	unsigned int cnt = 0;
	
	/* 1) Count the number of "one" bits in the 21-bit Encryption Counter.
	 * If this number is less than 10, go to "New Key-1". */
	encryption_counter = ((key_serial_number[7]&0x1f)<<16)|(key_serial_number[8]<<8)|key_serial_number[9];
	tmp = encryption_counter;
	while(tmp != 0)
	{
		if(tmp&0x01)
			cnt++;
		tmp = tmp >> 1;	
	}

	if(cnt < 10)
	{
		DAMO_DUKPT_New_Key1();
		return;
	}

	/* 2) Erase the key at ![Current Key Pointer]. */
	memset(current_key_pointer, 0, 16);

	/* 3) Set the LRC for ![Current Key Pointer] to an invalid value
	 * (e.g., increment the LRC by one). */
	current_key_pointer[16] = 0x01;

	/* 4) Add the Shift Register to the Encryption Counter. (This procedure skips
	 * those counter values that would have more than 10 "one" bits.) */
	encryption_counter += shift_register;

	key_serial_number[9] = encryption_counter & 0xff;
	key_serial_number[8] = (encryption_counter>>8) & 0xff;
	key_serial_number[7] = (key_serial_number[7] & 0xe0) | ((encryption_counter>>16) & 0x1f);
	
	/* 5) Go to "New Key-2". */
	DAMO_DUKPT_New_Key2();
}

static void DAMO_DUKPT_New_Key1()
{
	/* 1) Shift the Shift Register right one bit (end-off). (A "zero" is shifted into position #1,
	 * the leftmost bit of the register.) */
	shift_register_bit_num++;
	shift_register = shift_register >> 1;

	/* 2) If the Shift Register now contains all zeros (i.e., the single "one" was shifted off),
	 * go to "New Key-4", else go to "New Key-3" */
	if(shift_register)
	{
		DAMO_DUKPT_New_Key3();
	}
	else
	{
		DAMO_DUKPT_New_Key4();
	}
}

static void DAMO_DUKPT_New_Key2()
{
	/* 1) If the Encryption Counter contains all zeros, cease operation. (The PIN Entry Device is
	 * now inoperative, having encrypted more than 1 million PINs.) If not all zeros, go to "Exit" */
}

static void DAMO_DUKPT_New_Key3()
{
	/* 1) The Shift Register, right justified in 64 bits, padded to the left with zeros, OR'ed with the 64
	 * right-most bits of the Key Serial Number Register, is transferred into Crypto Register-1. */
	memcpy(crypto_register1, key_serial_number+2, 8);
	crypto_register1[4] |= ((shift_register >> 24) & 0xff);
	crypto_register1[5] |= ((shift_register >> 16) & 0xff);
	crypto_register1[6] |= ((shift_register >> 8) & 0xff);
	crypto_register1[7] |= (shift_register & 0xff);

	/* 2) Copy ![Current Key Pointer] into the Key Register. */
	memcpy(key_register, current_key_pointer, 16);

	/* 3) Call the subroutine "Non-reversible Key Generation Process" */
	/* 4) Store the contents of Crypto Register-1 into the left half of the Future Key Register
	 * indicated by the position of the single "one" bit in the Shift Register. */
	/* 5) Store the contents of Crypto Register-2 into the right half of the Future Key Register
	 * indicated by the position of the single "one" bit in the Shift Register. */
	DAMO_DUKPT_Gen_Non_Reversible_Key(key_register, crypto_register1, future_key_register[shift_register_bit_num]);

	/* 6) Generate and store the LRC on this Future Key Register. */
	future_key_register[shift_register_bit_num][16] = DAMO_DUKPT_Gen_LRC(future_key_register[shift_register_bit_num], 16); 

	/* 7) Go to "New Key-1". */
	DAMO_DUKPT_New_Key1();
}

static void DAMO_DUKPT_New_Key4()
{
#if 0
	unsigned int encryption_counter;
#endif

	/* 1) Erase the key at ![Current Key Pointer]. */
	memset(current_key_pointer, 0, 16);

	/* 2) Set the LRC for ![Current Key Pointer] to an invalid value (e.g., increment the LRC by one). */
	current_key_pointer[16] = 0x01;

	/* 3) Add one to the Encryption Counter. */
#if 1
	key_serial_number[9]++;
	if(key_serial_number[9] == 0)
	{
		key_serial_number[8]++;
		if(key_serial_number[8] == 0)
		{
			if((key_serial_number[7]&0x1f) != 0x1f)
			{
				key_serial_number[7]++;

			}
			else
			{
				key_serial_number[7]&=0xe0;
			}
		}
	}
#endif

#if 0
	encryption_counter = ((key_serial_number[7]&0x1f)<<16)|(key_serial_number[8]<<8)|key_serial_number[9];
	encryption_counter++;
	key_serial_number[9] = encryption_counter & 0xff;
	key_serial_number[8] = (encryption_counter>>8) & 0xff;
	key_serial_number[7] = (key_serial_number[7] & 0xe0) | ((encryption_counter>>16) & 0x1f);
#endif

	/* 4) Go to "New Key-2" */
	DAMO_DUKPT_New_Key2();
}

static void DAMO_DUKPT_Request_Pin_Entry1(int flags, DukptPinEntry *pEntry)
{
	unsigned char LRC;
	unsigned int encryption_counter;

	/* 1) Call the subroutine "Set Bit". */
	DAMO_DUKPT_Set_Bit();

	/* 2) Write into Current Key Pointer the address of that Future Key Register indicated 
	 * by the position of the "one" bit in the Shift Register. */
	current_key_pointer = future_key_register[shift_register_bit_num];

	LRC = DAMO_DUKPT_Gen_LRC(current_key_pointer, 16);

	if(LRC == current_key_pointer[16])
	{
		/* 3) Check the LRC on ![Current Key Pointer]. If this byte is correct (valid key),
		 * go to "Request PIN Entry 2". */
		DAMO_DUKPT_Request_Pin_Entry2(flags, pEntry);
	}
	else
	{
		/* 4) If the byte is incorrect, add the Shift Register to the Encryption Counter
		 * (to skip over the invalid key). */
		encryption_counter = ((key_serial_number[7]&0x1f)<<16)|(key_serial_number[8]<<8)|key_serial_number[9];
		encryption_counter += shift_register;
		if(!encryption_counter)
		{
			/* 5) If the Encryption Counter contains all zeros, cease operation. 
			 * (The PIN Entry Device is now inoperative, having encrypted more than 1 million PINs.) */
			return;
		}

		key_serial_number[9] = encryption_counter & 0xff;
		key_serial_number[8] = (encryption_counter>>8) & 0xff;
		key_serial_number[7] = (key_serial_number[7] & 0xe0) | ((encryption_counter>>16) & 0x1f);

		/* 6) Go to "Request PIN Entry 1". */
		DAMO_DUKPT_Request_Pin_Entry1(flags, pEntry);
	}
}

static void DAMO_DUKPT_Request_Pin_Entry2(int flags, DukptPinEntry *pEntry)
{
	/* 1) Copy ![Current Key Pointer] into the Key Register. */
	memcpy(key_register, current_key_pointer, 16);

	/* 2) (Optional: Perform this step if you need to generate a key that will be used in a message
	 * authentication process; this step does not affect the generation of the PIN encryption key)
	 * XOR the value in the Key Register with hexadecimal "0000 0000 0000 FF00 0000 0000 0000 FF00"
	 * and save this resultant key in the MAC key register. If a separate key is used to 
	 * verify the MAC response, XOR the value in the Key Register with hexadecimal
	 * "0000 0000 FF00 0000 0000 0000 FF00 0000" and save this resultant key
	 * in the MAC Response key register. */
	if( flags & DAMO_DUKPT_FLAG_USE_DATA_MAC_KEY )
		DAMO_DUKPT_Derive_MAK(key_register, pEntry);

	/* 3) (Optional: Perform this step if you need to generate a key that will be used in a data
	 * encryption process; this step does not affect the generation of the PIN encryption key)
	 * XOR the value in the Key Register with hexadecimal "0000 0000 00FF 0000 0000 0000 00FF 0000"
	 * The resultant key is encrypted using itself as the key - see the One Way Function in Figure A-2 
	 * in section A.4.1. Save this resultant key in the Data Encrypting key register. If a separate key is used to
	 * encrypt the transaction response, XOR the value in the Key Register with hexadecimal 
	 * "0000 00FF 0000 0000 0000 00FF 0000 0000" The resultant key is encrypted using itself as the key
	 * - see Figure A-2 in section A.4.1. Save this resultant key in the Data Encrypting Response key register.
	 * The input values into the One Way Function are not parity adjusted before they are used in the One Way Function. */
	if( flags & DAMO_DUKPT_FLAG_USE_DATA_ENC_KEY )
		DAMO_DUKPT_Derive_DEK(key_register, pEntry);

	if( flags & DAMO_DUKPT_FLAG_USE_PIN_ENC_KEY )
    DAMO_DUKPT_Derive_PEK(key_register, pEntry);

  /* 4) XOR the Key Register with hexadecimal "0000 0000 0000 00FF 0000 0000 0000 00FF" (This will produce a variant of the key.) */
  key_register[7]^=0xff;
  key_register[15]^=0xff;

  /* 5) Call the subroutine "Triple-DEA Encrypt" */
  DAMO_DUKPT_Triple_DEA_Encrypt();

  /* 6) Format and transmit the encrypted PIN block response message, which includes:
  * - The data in the Key Serial Number Register with leading hexadecimal "F's" suppressed (includes the 21-bit Encryption Counter).
  * - The encrypted PIN block in Crypto Register-1. */
  memcpy(pEntry->ksn, key_serial_number, 10);
  memcpy(pEntry->enc_pin_block, crypto_register1, 8);
	
	/* 7) Go to "New Key". */
	DAMO_DUKPT_New_Key();
}

void DAMO_DUKPT_Gen_Non_Reversible_Key(unsigned char key[16], const unsigned char in[8], unsigned char out[16])
{
	unsigned char key2[16];

	memcpy(key2, key, 16);

	/* 1) Crypto Register-1 XORed with the right half of the Key Register goes to Crypto Register-2. */
	/* 2) Crypto Register-2 DEA-encrypted using, as the key, the left half of the Key Register goes to Crypto Register-2. */
	/* 3) Crypto Register-2 XORed with the right half of the Key Register goes to Crypto Register-2. */
	DAMO_DUKPT_Crypto_Register(key2, in, out+8);

	/* 4) XOR the Key Register with hexadecimal C0C0 C0C0 0000 0000 C0C0 C0C0 0000 0000. */
	key2[0]^=0xc0; key2[1]^=0xc0; key2[2]^=0xc0; key2[3]^=0xc0;
	key2[8]^=0xc0; key2[9]^=0xc0; key2[10]^=0xc0; key2[11]^=0xc0;

	/* 5) Crypto Register-1 XORed with the right half of the Key Register goes to Crypto Register-1. */
	/* 6) Crypto Register-1 DEA-encrypted using, as the key, the left half of the Key Register goes to Crypto Register-1. */
	/* 7) Crypto Register-1 XORed with the right half of the Key Register goes to Crypto Register-1. */
	DAMO_DUKPT_Crypto_Register(key2, in, out);

  memset(key2, 0x00, sizeof(key2));
		
	/* 8) Return from subroutine. */
}

void DAMO_DUKPT_Derive_PEK(unsigned char key[16], DukptPinEntry *pEntry)
{
	memcpy(pEntry->pin_enc_key, key, 16);
	pEntry->pin_enc_key[7]^=0xff;
	pEntry->pin_enc_key[15]^=0xff;
}

void DAMO_DUKPT_Derive_MAK(unsigned char key[16], DukptPinEntry *pEntry)
{
	memcpy(pEntry->req_mac_key, key, 16);
	pEntry->req_mac_key[6]^=0xff;
	pEntry->req_mac_key[14]^=0xff;
	memcpy(pEntry->res_mac_key, key, 16);
	pEntry->res_mac_key[4]^=0xff;
	pEntry->res_mac_key[12]^=0xff;
}

void DAMO_DUKPT_Derive_DEK(unsigned char key[16], DukptPinEntry *pEntry)
{
	unsigned char variant_key[16];
	DES3_CTX ctx;

	memcpy(variant_key, key, 16);
	variant_key[5]^=0xff;
	variant_key[13]^=0xff;
	DAMO_CRYPT_DES3_Set_EKey2( &ctx, variant_key );
	DAMO_CRYPT_DES3_Block( &ctx, variant_key, pEntry->req_enc_key );
	DAMO_CRYPT_DES3_Block( &ctx, variant_key+8, pEntry->req_enc_key+8 );

	memcpy(variant_key, key, 16);
	variant_key[3]^=0xff;
	variant_key[11]^=0xff;
	DAMO_CRYPT_DES3_Set_EKey2( &ctx, variant_key );
	DAMO_CRYPT_DES3_Block( &ctx, variant_key, pEntry->res_enc_key );
	DAMO_CRYPT_DES3_Block( &ctx, variant_key+8, pEntry->res_enc_key+8 );

  memset(variant_key, 0x00, sizeof(variant_key));
}

void Print_Future_Key_Register()
{
	int i, j;

	printf("********** Future Key Register **********\n");
	for(i=0; i<21; i++)
	{
		printf("%d-", i);
		for(j=0; j<16; j++)
			printf("%02x", future_key_register[i][j]);
		printf("-%02x\n", future_key_register[i][j]);
	}
	printf("*****************************************\n");
}

int DAMO_DUKPT_Export_Future_Key_Info(DukptFutureKeyInfo *fki)
{
  return DAMO_DUKPT_Export_Future_Key_Info_Ek(NULL, fki);
}
int DAMO_DUKPT_Export_Future_Key_Info_Ek(unsigned char *external_key, DukptFutureKeyInfo *fki)
{
	int i;
	int ret;
	size_t enc_ksn_len = 17;
	unsigned char in[26];
	unsigned char out[33];
	size_t out_len = 0 ;
	size_t enc_fk_len = 45;
	unsigned char key1[AP_KEY_SIZE];
	size_t key1_len = 16;
  unsigned char internal_key[16] = {0x7a, 0x0a, 0x60, 0xe1, 0xe8, 0x66, 0x8d, 0x41, 0xbf, 0x93, 0x97, 0x82, 0x30, 0x48, 0x30, 0x99};
  static unsigned char pre_shared_key[16]={0};

  if( !external_key )
    memcpy(pre_shared_key, internal_key, 16);
  else
    memcpy(pre_shared_key, external_key, 16);

	memset(fki, 0x00, sizeof(DukptFutureKeyInfo));
	/* Print_Future_Key_Register(); */

	ret = DAMO_CRYPT_Base64_Encode(fki->key_serial_number, &enc_ksn_len, key_serial_number, 10 );
	if(ret < 0)
	{
		return ret;
	}

	memcpy(in, pre_shared_key, AP_PSK_SIZE);
	memset(pre_shared_key, 0x00, sizeof(pre_shared_key));
	memcpy(in+AP_PSK_SIZE, key_serial_number, 10);

	if(!DAMO_CRYPT_KDF2_SHA256(in, 26, key1, &key1_len))
	{
		memset(in, 0x00, sizeof(in));
		ret = DAMO_DUKPT_ERR_AP_GTK_GET_TKEY_FAIL;
		return ret;
	}
	memset(in, 0x00, sizeof(in));

	for(i=0; i<21; i++)
	{
		ret = DAMO_CRYPT_AES_Encrypt(out, &out_len, future_key_register[i], 17, key1, AP_KEY_SIZE, AES_128, CBC_MODE);
		if(ret < 0)
    {
      memset(key1, 0x00, sizeof(key1));
			return ret;
    }
		ret = DAMO_CRYPT_Base64_Encode(fki->future_key_register[i], &enc_fk_len, out, out_len );
		if(ret < 0)
    {
      memset(key1, 0x00, sizeof(key1));
			return ret;
    }
		enc_fk_len = 45;
		memset(future_key_register[i], 0x00, sizeof(future_key_register[i]));
	}

  memset(key1, 0x00, sizeof(key1));
  memset(key_register, 0x00, sizeof(key_register));

	return ret;
}

int DAMO_DUKPT_Import_Future_Key_Info(DukptFutureKeyInfo *fki)
{
	return DAMO_DUKPT_Import_Future_Key_Info_Ek(NULL, fki);
}

int DAMO_DUKPT_Import_Future_Key_Info_Ek(unsigned char *external_key, DukptFutureKeyInfo *fki)
{
	int i;
	int ret;
	size_t dec_ksn_len = 10;
	unsigned char in[26];
	unsigned char out[33];
	size_t out_len = 33 ;
	size_t dec_fk_len = 17;
	unsigned char key1[AP_KEY_SIZE];
	size_t key1_len = 16;
  unsigned char internal_key[16] = {0x7a, 0x0a, 0x60, 0xe1, 0xe8, 0x66, 0x8d, 0x41, 0xbf, 0x93, 0x97, 0x82, 0x30, 0x48, 0x30, 0x99};
  static unsigned char pre_shared_key[16]= {0};
  DukptFutureKeyInfoOld fkiOld;
  int encType = 0; /* 0:CBC, 1:CFB(DukptFutureKeyInfoOld) */
  unsigned char decTemp[33];

  if ( !external_key )
    memcpy(pre_shared_key, internal_key, 16);
  else
    memcpy(pre_shared_key, external_key, 16);

  /* Check CFB/CBC encrypted data */
  memcpy(&fkiOld, fki, sizeof(DukptFutureKeyInfoOld));
	for(i=0; i<21; i++)
	{
    if ( ( fkiOld.future_key_register[i][0] != 0x00) && (fkiOld.future_key_register[i][24] == 0x00) )
      encType = 1;
    else
    {
      encType = 0;
      break;
    }
  }

  if ( encType == 0)
  	ret = DAMO_CRYPT_Base64_Decode(key_serial_number, &dec_ksn_len, fki->key_serial_number, 16 );
  else 
  	ret = DAMO_CRYPT_Base64_Decode(key_serial_number, &dec_ksn_len, fkiOld.key_serial_number, 16 );
	if(ret < 0)
	{
		return ret;
	}
	memcpy(in, pre_shared_key, AP_PSK_SIZE);
	memset(pre_shared_key, 0x00, sizeof(pre_shared_key));
	memcpy(in+AP_PSK_SIZE, key_serial_number, 10);

	if(!DAMO_CRYPT_KDF2_SHA256(in, 26, key1, &key1_len))
	{
		memset(in, 0x00, sizeof(in));
		ret = DAMO_DUKPT_ERR_AP_GTK_GET_TKEY_FAIL;
		return ret;
	}
	memset(in, 0x00, sizeof(in));

	for(i=0; i<21; i++)
	{
    if ( encType == 0)
  		ret = DAMO_CRYPT_Base64_Decode(out, &out_len, fki->future_key_register[i], 44);
    else 
  		ret = DAMO_CRYPT_Base64_Decode(out, &out_len, fkiOld.future_key_register[i], 24);
		if(ret < 0)
    {
      memset(decTemp, 0x00, sizeof(decTemp));
      memset(key1, 0x00, sizeof(key1));
			return ret;
    }
    if ( encType == 0)
  		ret = DAMO_CRYPT_AES_Decrypt(decTemp, &dec_fk_len, out, out_len, key1, AP_KEY_SIZE, AES_128, CBC_MODE);
    else
  		ret = DAMO_CRYPT_AES_Decrypt(decTemp, &dec_fk_len, out, out_len, key1, AP_KEY_SIZE, AES_128, CFB_MODE);
		if(ret < 0)
    {
      memset(decTemp, 0x00, sizeof(decTemp));
      memset(key1, 0x00, sizeof(key1));
			return ret;
    }
    memcpy(future_key_register[i], decTemp, dec_fk_len);
		dec_fk_len = 17;
	}
	/* Print_Future_Key_Register(); */

	DAMO_DUKPT_Power_On_Reset();

  memset(key1, 0x00, sizeof(key1));
  memset(decTemp, 0x00, sizeof(decTemp));
	memset(pre_shared_key, 0x00, sizeof(pre_shared_key));

	return ret;
}

LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Load_Initial_Key(const unsigned char *ipek, size_t ipek_len, const unsigned char *ksn, size_t ksn_len)
{
  unsigned char ksn10[10];
  int ret = 0;

  if(!ipek || !ksn)
		return DAMO_DUKPT_ERR_CLI_LIK_NULL_POINTER;

	if(ipek_len < 16 || ksn_len < 10)
		return DAMO_DUKPT_ERR_CLI_LIK_INVALID_LEN;

  ret = DAMO_DUKPT_Make_KSN10(ksn, ksn_len, ksn10, sizeof(ksn10));
  if ( ret !=  DAMO_DUKPT_SUCCESS )
    return ret;

	/* 1) Store the initial PIN encryption key, as received in the externally initiated command, 
	 * into Future Key Register #21. */
	memcpy(future_key_register[20],  ipek, 16);

	/* 2) Generate and store the LRC on this Future Key Register. */
	future_key_register[20][16] = DAMO_DUKPT_Gen_LRC(ipek, 16);

	/* 3) Write the address of Future Key Register #21 into the Current Key Pointer. */
	current_key_pointer = future_key_register[20];

	/* 4) Store the Key Serial Number, as received in the externally initiated command, into the Key
	 * Serial Number Register. (This register is the concatenation of the Initial Key Serial
	 * Number Register and the Encryption Counter.) */
	memcpy(key_serial_number, ksn10, 10);

	/* 5) Clear the Encryption Counter (the 21 right-most bits of the Key Serial Number Register). */
	key_serial_number[7]&=0xe0;
	key_serial_number[8]=0;
	key_serial_number[9]=0;

	/* 6) Set bit #1 (the left-most bit) of the Shift Register to "one", setting all of the other bits to "zero". */
	shift_register_bit_num = 0;
	shift_register = 0x100000;

	/* 7) Go to "New Key-3". */
	DAMO_DUKPT_New_Key3();

	return DAMO_DUKPT_SUCCESS;
}

LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Request_Pin_Entry(const unsigned char *pin, size_t pin_len,
		const unsigned char *account_num, size_t account_num_len, int flags, DukptPinEntry *pEntry)
{
	unsigned char clear_pin_block[8];
	unsigned char tmp[8]={0x00,};

	int i;
	int index;
	int sub_result;

	if(!pin || !account_num || !pEntry)
		return DAMO_DUKPT_ERR_CLI_RPE_NULL_POINTER;

	if(account_num_len < 13) 
		return DAMO_DUKPT_ERR_CLI_RPE_INVALID_LEN;

  /* if dukpt maximum enc count, return error */
  if( (pEntry->ksn[9] == 0x00) && (pEntry->ksn[8] == 0xf8) && (pEntry->ksn[7]&0x1f) == 0x1f)
    return DAMO_DUKPT_ERR_CLI_CODE_BASE_REQUEST_ENTRY + DAMO_DUKPT_ERR_CLI_MAXIMUM_COUNT;

	/* 1) Transfer the primary account number as received in 
	 * the externally initiated command into the Account Number Register. */
	
	/* 2) Activate the PIN Entry Device keyboard and the Enter key. */
	
	/* 3) If the PIN is not entered, send the encrypted PIN block response message
	 * without the PIN-related data elements and go to "Exit". */
		
	/* 4) If the PIN is entered, use the cardholder-entered PIN and the primary account number to
	 * generate the cleartext PIN block and store it in Crypto Register-1. */

	if( flags & DAMO_DUKPT_FLAG_USE_PIN_ENC_KEY )
	{
		/* ISO-0 PIN-block format The ISO-0 PIN-block format consists of the PIN length, PIN digits, and 'F' character padding, 
		 * exclusive-ORed with the rightmost 12 PAN digits, excluding the check digit. */
		memset(clear_pin_block, 0xff, sizeof(clear_pin_block));
		clear_pin_block[0] = (unsigned char) pin_len;

		for(i=1; i<8; i++)
		{
			index = 2*i-2;

			sub_result = (int)pin_len - index;
			if(sub_result != 1)
			{
				clear_pin_block[i] = ((pin[index]-0x30) << 4) | (pin[index+1]-0x30);
				if(sub_result == 2) break;
			}
			else
			{
				clear_pin_block[i] = ((pin[index]-0x30) << 4) | 0x0f;
				break;
			}
		}

		for(i=7; i>=2; i--)
		{
			index = (int)account_num_len -(2*(8-i)+1);
			tmp[i] = ((account_num[index]-0x30) << 4) | (account_num[index+1]-0x30);
		}

		crypto_register1[0] = clear_pin_block[0]^tmp[0];
		crypto_register1[1] = clear_pin_block[1]^tmp[1];
		crypto_register1[2] = clear_pin_block[2]^tmp[2];
		crypto_register1[3] = clear_pin_block[3]^tmp[3];
		crypto_register1[4] = clear_pin_block[4]^tmp[4];
		crypto_register1[5] = clear_pin_block[5]^tmp[5];
		crypto_register1[6] = clear_pin_block[6]^tmp[6];
		crypto_register1[7] = clear_pin_block[7]^tmp[7];
	}
	
	/* 5) Go to "Request PIN Entry 1". */
	DAMO_DUKPT_Request_Pin_Entry1(flags, pEntry);

  /* if enc count > 10,00,000, return warning message */
  if( ((pEntry->ksn[9] >= 0x2D) && (pEntry->ksn[8] == 0x62) && ((pEntry->ksn[7]&0x1C) == 0x1C)) ||
    ((pEntry->ksn[8] > 0x62) && ((pEntry->ksn[7]&0x1C) == 0x1C)) || (((pEntry->ksn[7]&0x1F) >= 0x1D)) )
      return DAMO_DUKPT_ERR_CLI_CODE_BASE_REQUEST_ENTRY + DAMO_DUKPT_WARNING_CLI_LIMIT_COUNT;

	return DAMO_DUKPT_SUCCESS;
}

void DAMO_DUKPT_Power_On_Reset()
{
	int i;
	unsigned char LRC;
	unsigned int encryption_counter=0;

	/* 1) Set to "one" those bits of the Encryption Counter that correspond to the Future Key
	 * Registers lost because of the power interruption. */
	for(i=0; i<21; i++)
	{
		LRC = DAMO_DUKPT_Gen_LRC(future_key_register[i], 16);
		if(LRC != future_key_register[i][16])
		{
			encryption_counter |= (1<<(20-i));
		}
	}

	/* 2) Increment the Encryption Counter. */
	encryption_counter++;
	key_serial_number[9] = encryption_counter & 0xff;
	key_serial_number[8] = (encryption_counter>>8) & 0xff;
	key_serial_number[7] = (key_serial_number[7] & 0xe0) | ((encryption_counter>>16) & 0x1f);
}

int DAMO_DUKPT_AES_Encrypt(unsigned char *enc, size_t *enc_len, unsigned char *KSN, size_t *KSN_len, const unsigned char *org, size_t org_len, DukptFutureKeyInfo *fki)
{
  unsigned char pin[4] = {0x4C, };
	unsigned char account_num[16] = {0x4C, };
	DukptPinEntry pEntry;
	int flags = DAMO_DUKPT_FLAG_USE_ALL_KEY;
	int ret=0;
	size_t tmp_enc_len=0;

	/* import ksn, future key */
	if(DAMO_DUKPT_Import_Future_Key_Info(fki)<0)
		if (ret != DAMO_DUKPT_SUCCESS)
			return ret;

	ret = DAMO_DUKPT_Request_Pin_Entry(pin, sizeof(pin), account_num, sizeof(account_num), flags, &pEntry);
	if (ret != DAMO_DUKPT_SUCCESS)
		return ret;

	ret = DAMO_CRYPT_AES_Encrypt(enc, &tmp_enc_len, org, org_len, pEntry.req_enc_key, 16, AES_128, CBC_MODE);
	if (ret != DAMO_DUKPT_SUCCESS)
		return ret;

	/* export ksn, future key */
	if(DAMO_DUKPT_Export_Future_Key_Info(fki)<0)
		if (ret != DAMO_DUKPT_SUCCESS)
			return ret;

	/* output */
	*enc_len = tmp_enc_len;
	memcpy(KSN, pEntry.ksn, 10);
	*KSN_len = 10;
  memset(&pEntry, 0x00, sizeof(pEntry));

	return DAMO_DUKPT_SUCCESS;
}

LIBSPEC_DUKPTCLIENT
#if 0		// SPARROW : Original Code
int DAMO_DUKPT_Client_PK_Auth_Start(unsigned char *publicKeyFilePath, unsigned char *SrandData, unsigned char *HashData, unsigned char *SignValue, unsigned char OutRandValue[16], unsigned char *OutEncRandValue)
{
  int ret =0;

  unsigned char srandDataByte[32];
  unsigned char hashDataByte[32];
  unsigned char signValueByte[256];
  int srandDataLen = 0;
  int hashDataLen  = 0;
  int signValueLen = 0;

  int dataLen = 0;
  unsigned char plainDataByte[64];
  int plainDataLen = 0;

  unsigned char outEncDataByte[256];
  size_t outEncDataLen = 0;

  const int error_base = DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_START;

  /* Check Input Data */
  if ( publicKeyFilePath == NULL || SrandData == NULL || HashData == NULL || SignValue == NULL || OutRandValue == NULL || OutEncRandValue == NULL )
    return error_base + DAMO_DUKPT_ERR_CLI_NULL_POINTER;

  ret = DAMO_DUKPT_HexToByte(srandDataByte, &srandDataLen, SrandData, (int)strlen(SrandData));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;
  ret = DAMO_DUKPT_HexToByte(hashDataByte,  &hashDataLen,  HashData,  (int)strlen(HashData));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;
  ret = DAMO_DUKPT_HexToByte(signValueByte, &signValueLen, SignValue, (int)strlen(SignValue));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;

  if ( srandDataLen != 32 || hashDataLen != 32 || signValueLen != 256 || (int)strlen(publicKeyFilePath) <= 0 )
    return error_base +  DAMO_DUKPT_ERR_CLI_INVALID_INPUT_LEN;

  /* verify signValueByte */
  ret = DAMO_PKCRYPT_pk_verify( publicKeyFilePath, hashDataByte, hashDataLen, signValueByte, signValueLen);
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base - ret;

  /* create OutRandValue */
  ret = DAMO_CRYPT_RNG( OutRandValue, 16 );
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base + ret;

  /* create plainDataByte ( SHA256( RandValue Length(2byte) + RandValue(16byte), 32byte ) + RandValue Length(2) + RandValue(16), 50byte) */

#ifdef DUKPT_CLIENT_PKAUTH_STRING
  plainDataByte[32] = '1';
  plainDataByte[33] = '6'; 
#else
  plainDataByte[32] = 0x00;
  plainDataByte[33] = 16;
#endif
 
  memcpy(&plainDataByte[34], OutRandValue, 16);
  plainDataLen = 2 + 16;

  ret = DAMO_CRYPT_HASH_SHA256(&plainDataByte[32], plainDataLen, hashDataByte);
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base + ret;

  memcpy(plainDataByte, hashDataByte, hashDataLen);
  plainDataLen += hashDataLen;

#ifdef _DEBUG_PRINT
{
  int i=0;
  printf("plainDataByte(%d) = ", plainDataLen);
  for ( i=0; i<plainDataLen; i++)
    printf("%02X", plainDataByte[i]&0xff);
  printf("\n");
}
#endif

  /* publicKey encrypt plainDataByte */
  ret = DAMO_PKCRYPT_pk_encrypt( publicKeyFilePath, plainDataByte, plainDataLen, outEncDataByte, &outEncDataLen, sizeof(outEncDataByte));
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base - ret;

  DAMO_DUKPT_ByteToHex(OutEncRandValue, &dataLen, outEncDataByte, (int)outEncDataLen);

  return DAMO_DUKPT_SUCCESS;
}
#else
int DAMO_DUKPT_Client_PK_Auth_Start(unsigned char *publicKeyFilePath, unsigned char *SrandData, unsigned char *HashData, unsigned char *SignValue, unsigned char OutRandValue[16], unsigned char *OutEncRandValue)
{
  int ret =0;

  unsigned char srandDataByte[32];
  unsigned char hashDataByte[32];
  unsigned char signValueByte[256];
  int srandDataLen = 0;
  int hashDataLen  = 0;
  int signValueLen = 0;

  int dataLen = 0;
  unsigned char plainDataByte[64];
  int plainDataLen = 0;

  unsigned char outEncDataByte[256];
  size_t outEncDataLen = 0;

  const int error_base = DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_START;

  /* Check Input Data */
  if ( publicKeyFilePath == NULL || SrandData == NULL || HashData == NULL || SignValue == NULL || OutRandValue == NULL || OutEncRandValue == NULL )
    return error_base + DAMO_DUKPT_ERR_CLI_NULL_POINTER;

  ret = DAMO_DUKPT_HexToByte(srandDataByte, &srandDataLen, (char*)SrandData, (int)strlen((char const*)SrandData));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;
  ret = DAMO_DUKPT_HexToByte(hashDataByte,  &hashDataLen,  (char*)HashData,  (int)strlen((char const*)HashData));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;
  ret = DAMO_DUKPT_HexToByte(signValueByte, &signValueLen, (char*)SignValue, (int)strlen((char const*)SignValue));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;

  //if ( srandDataLen != 32 || hashDataLen != 32 || signValueLen != 256 || (int)strlen(publicKeyFilePath) <= 0 )
	if ( srandDataLen != 32 || hashDataLen != 32 || signValueLen != 256 )
    return error_base +  DAMO_DUKPT_ERR_CLI_INVALID_INPUT_LEN;

  /* verify signValueByte */
  ret = DAMO_PKCRYPT_pk_verify( (char*)publicKeyFilePath, hashDataByte, hashDataLen, signValueByte, signValueLen);
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base - ret;

  /* create OutRandValue */
  ret = DAMO_CRYPT_RNG( OutRandValue, 16 );
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base + ret;

  /* create plainDataByte ( SHA256( RandValue Length(2byte) + RandValue(16byte), 32byte ) + RandValue Length(2) + RandValue(16), 50byte) */

#ifdef DUKPT_CLIENT_PKAUTH_STRING
  plainDataByte[32] = '1';
  plainDataByte[33] = '6'; 
#else
  plainDataByte[32] = 0x00;
  plainDataByte[33] = 16;
#endif
 
  memcpy(&plainDataByte[34], OutRandValue, 16);
  plainDataLen = 2 + 16;

  ret = DAMO_CRYPT_HASH_SHA256(&plainDataByte[32], plainDataLen, hashDataByte);
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base + ret;

  memcpy(plainDataByte, hashDataByte, hashDataLen);
  plainDataLen += hashDataLen;

#ifdef _DEBUG_PRINT
{
  int i=0;
  printf("plainDataByte(%d) = ", plainDataLen);
  for ( i=0; i<plainDataLen; i++)
    printf("%02X", plainDataByte[i]&0xff);
  printf("\n");
}
#endif

  /* publicKey encrypt plainDataByte */
  ret = DAMO_PKCRYPT_pk_encrypt( (char*)publicKeyFilePath, plainDataByte, plainDataLen, outEncDataByte, &outEncDataLen, sizeof(outEncDataByte));
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base - ret;

  DAMO_DUKPT_ByteToHex((char*)OutEncRandValue, &dataLen, outEncDataByte, (int)outEncDataLen);

  return DAMO_DUKPT_SUCCESS;
}
#endif

LIBSPEC_DUKPTCLIENT
#if 0		// SPARROW : Original Code
int DAMO_DUKPT_Client_PK_Auth_End(unsigned char *publicKeyFilePath, unsigned char *EncIpekValue, unsigned char *ksn, unsigned char randValueByte[16], unsigned char IpekValue[16])
{
  int ret =0;

  unsigned char encIpekValueByte[128];
  int encIpekValueLen = 0;

  unsigned char encKey[16] = { 0 };
  unsigned char macKey[16] = { 0 };

  unsigned char decIpekValueByte[128];
  size_t decIpekValueLen = 0;

  unsigned char macKeyEncValue[128];
  size_t macKeyEncValueLen = 0;

  unsigned char ivec[16] = {0};

  unsigned char hashDataByte[32];

  const int error_base = DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_END;

  /* Check input data */
  if ( publicKeyFilePath == NULL || EncIpekValue == NULL || randValueByte == NULL || IpekValue == NULL)
    return error_base + DAMO_DUKPT_ERR_CLI_NULL_POINTER;

  ret = DAMO_DUKPT_HexToByte(encIpekValueByte, &encIpekValueLen, EncIpekValue, (int)strlen(EncIpekValue));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;

  if ( encIpekValueLen != 72 || strlen(publicKeyFilePath) <= 0 )
    return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_LEN;

  /* Create encKey, macKey by x9 tr31 */
  DAMO_X9TR31_Get_Enc_Mac_Key( randValueByte, encKey, macKey );
#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("randomKey = ");
    for ( i=0; i<16; i++)
      printf("%02X", randValueByte[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("encKey = ");
    for ( i=0; i<16; i++)
      printf("%02X", encKey[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("macKey = ");
    for ( i=0; i<16; i++)
      printf("%02X", macKey[i]&0xff);
    printf("\n");
  }
#endif

#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("encIpekValueByte(%d) = ", encIpekValueLen);
    for ( i=0; i<encIpekValueLen; i++)
      printf("%02X", encIpekValueByte[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("ivec(%d) = ", 16);
    for ( i=0; i<16; i++)
      printf("%02X", ivec[i]&0xff);
    printf("\n");
  }
#endif

  /* Check MacData */
  ret = DAMO_CRYPT_AES_EncryptEx(macKeyEncValue, &macKeyEncValueLen, encIpekValueByte, encIpekValueLen-8, macKey, sizeof(macKey), AES_128, CBC_MODE, ivec);
  if ( ret != DAMO_CRYPT_SUCCESS )
    return error_base + ret;

#ifdef _DEBUG_PRINT
	printf("macKeyEncValue = %02X%02X%02X%02X%02X%02X%02X%02X\n", macKeyEncValue[0]&0xff, macKeyEncValue[1]&0xff, macKeyEncValue[2]&0xff, macKeyEncValue[3]&0xff, macKeyEncValue[4]&0xff, macKeyEncValue[5]&0xff, macKeyEncValue[6]&0xff, macKeyEncValue[7]&0xff);
#endif

  if ( memcmp(macKeyEncValue, &encIpekValueByte[64], 8) != 0 )
    return error_base + DAMO_CRYPT_ERR_CLI_PK_AUTH_END_VERIFY_MAC;

#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("encIpekValueByte(%d) = ", encIpekValueLen);
    for ( i=0; i<encIpekValueLen; i++)
      printf("%02X", encIpekValueByte[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("ivec(%d) = ", 16);
    for ( i=0; i<16; i++)
      printf("%02X", ivec[i]&0xff);
    printf("\n");
  }
#endif

  /* Get IPEK ( decrypt encIpekValueByte ) */
  ret = DAMO_CRYPT_AES_DecryptEx(decIpekValueByte, &decIpekValueLen, encIpekValueByte, encIpekValueLen-8, encKey, sizeof(encKey), AES_128, CBC_MODE, ivec);
  if ( ret != DAMO_CRYPT_SUCCESS )
    return error_base + ret;
#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("decIpekValueByte(%d) = ", decIpekValueLen);
    for ( i=0; i<(int)decIpekValueLen; i++)
      printf("%02X", decIpekValueByte[i]&0xff);
    printf("\n");
  }
#endif

  /* decIpekValueByte ( SHA256(KSN(10byte) + IPEK Length(2byte) + IPEK(16byte), 32byte) + KSN(10byte) + IPEK Length(2byte) + IPEK(16byte),  60byte) */
  if ( memcmp(&decIpekValueByte[32], ksn, 10) != 0)
    return error_base + DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_KSN;
	
#ifdef DUKPT_CLIENT_PKAUTH_STRING
  if ( decIpekValueByte[42] != '1' && decIpekValueByte[43] != '6')
#else
  if ( decIpekValueByte[43] != 16)
#endif

    return error_base + DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_IPEK_LENGTH;

  ret = DAMO_CRYPT_HASH_SHA256(&decIpekValueByte[32], 28, hashDataByte);
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base + ret;
#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("hashDataByte = ");
    for ( i=0; i<32; i++)
      printf("%02X", hashDataByte[i]&0xff);
    printf("\n");
  }
#endif

  if ( memcmp(hashDataByte, decIpekValueByte, 32) != 0 )
    return error_base + DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_HASH;

	memcpy(IpekValue, &decIpekValueByte[44], 16);
  
  memset(encKey, 0x00, sizeof(encKey));
  memset(macKey, 0x00, sizeof(macKey));

  return DAMO_DUKPT_SUCCESS;
}
#else
int DAMO_DUKPT_Client_PK_Auth_End(unsigned char *publicKeyFilePath, unsigned char *EncIpekValue, unsigned char *ksn, unsigned char randValueByte[16], unsigned char IpekValue[16])
{
  int ret =0;

  unsigned char encIpekValueByte[128];
  int encIpekValueLen = 0;

  unsigned char encKey[16] = { 0 };
  unsigned char macKey[16] = { 0 };

  unsigned char decIpekValueByte[128];
  size_t decIpekValueLen = 0;

  unsigned char macKeyEncValue[128];
  size_t macKeyEncValueLen = 0;

  unsigned char ivec[16] = {0};

  unsigned char hashDataByte[32];

  const int error_base = DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_END;

  /* Check input data */
  if ( publicKeyFilePath == NULL || EncIpekValue == NULL || randValueByte == NULL || IpekValue == NULL)
    return error_base + DAMO_DUKPT_ERR_CLI_NULL_POINTER;

  ret = DAMO_DUKPT_HexToByte(encIpekValueByte, &encIpekValueLen, (char*)EncIpekValue, (int)strlen((char const *)EncIpekValue));
	if ( ret != DAMO_CRYPT_SUCCESS )
		return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;

   //if ( encIpekValueLen != 72 || strlen(publicKeyFilePath) <= 0 )
	if ( encIpekValueLen != 72  )
    return error_base + DAMO_DUKPT_ERR_CLI_INVALID_INPUT_LEN;

  /* Create encKey, macKey by x9 tr31 */
  DAMO_X9TR31_Get_Enc_Mac_Key( randValueByte, encKey, macKey );
#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("randomKey = ");
    for ( i=0; i<16; i++)
      printf("%02X", randValueByte[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("encKey = ");
    for ( i=0; i<16; i++)
      printf("%02X", encKey[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("macKey = ");
    for ( i=0; i<16; i++)
      printf("%02X", macKey[i]&0xff);
    printf("\n");
  }
#endif

#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("encIpekValueByte(%d) = ", encIpekValueLen);
    for ( i=0; i<encIpekValueLen; i++)
      printf("%02X", encIpekValueByte[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("ivec(%d) = ", 16);
    for ( i=0; i<16; i++)
      printf("%02X", ivec[i]&0xff);
    printf("\n");
  }
#endif

  /* Check MacData */
  ret = DAMO_CRYPT_AES_EncryptEx(macKeyEncValue, &macKeyEncValueLen, encIpekValueByte, encIpekValueLen-8, macKey, sizeof(macKey), AES_128, CBC_MODE, ivec);
  if ( ret != DAMO_CRYPT_SUCCESS )
    return error_base + ret;

#ifdef _DEBUG_PRINT
	printf("macKeyEncValue = %02X%02X%02X%02X%02X%02X%02X%02X\n", macKeyEncValue[0]&0xff, macKeyEncValue[1]&0xff, macKeyEncValue[2]&0xff, macKeyEncValue[3]&0xff, macKeyEncValue[4]&0xff, macKeyEncValue[5]&0xff, macKeyEncValue[6]&0xff, macKeyEncValue[7]&0xff);
#endif

  if ( memcmp(macKeyEncValue, &encIpekValueByte[64], 8) != 0 )
    return error_base + DAMO_CRYPT_ERR_CLI_PK_AUTH_END_VERIFY_MAC;

#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("encIpekValueByte(%d) = ", encIpekValueLen);
    for ( i=0; i<encIpekValueLen; i++)
      printf("%02X", encIpekValueByte[i]&0xff);
    printf("\n");
  }
  {
    int i=0;
    printf("ivec(%d) = ", 16);
    for ( i=0; i<16; i++)
      printf("%02X", ivec[i]&0xff);
    printf("\n");
  }
#endif

  /* Get IPEK ( decrypt encIpekValueByte ) */
  ret = DAMO_CRYPT_AES_DecryptEx(decIpekValueByte, &decIpekValueLen, encIpekValueByte, encIpekValueLen-8, encKey, sizeof(encKey), AES_128, CBC_MODE, ivec);
  if ( ret != DAMO_CRYPT_SUCCESS )
    return error_base + ret;
#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("decIpekValueByte(%d) = ", decIpekValueLen);
    for ( i=0; i<(int)decIpekValueLen; i++)
      printf("%02X", decIpekValueByte[i]&0xff);
    printf("\n");
  }
#endif

  /* decIpekValueByte ( SHA256(KSN(10byte) + IPEK Length(2byte) + IPEK(16byte), 32byte) + KSN(10byte) + IPEK Length(2byte) + IPEK(16byte),  60byte) */
  if ( memcmp(&decIpekValueByte[32], ksn, 10) != 0)
    return error_base + DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_KSN;
	
#ifdef DUKPT_CLIENT_PKAUTH_STRING
  if ( decIpekValueByte[42] != '1' && decIpekValueByte[43] != '6')
#else
  if ( decIpekValueByte[43] != 16)
#endif

    return error_base + DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_IPEK_LENGTH;

  ret = DAMO_CRYPT_HASH_SHA256(&decIpekValueByte[32], 28, hashDataByte);
  if ( ret != DAMO_DUKPT_SUCCESS )
    return error_base + ret;
#ifdef _DEBUG_PRINT
  {
    int i=0;
    printf("hashDataByte = ");
    for ( i=0; i<32; i++)
      printf("%02X", hashDataByte[i]&0xff);
    printf("\n");
  }
#endif

  if ( memcmp(hashDataByte, decIpekValueByte, 32) != 0 )
    return error_base + DAMO_DUKPT_ERR_CLI_PK_AUTH_END_VERIFY_HASH;

	memcpy(IpekValue, &decIpekValueByte[44], 16);
  
  memset(encKey, 0x00, sizeof(encKey));
  memset(macKey, 0x00, sizeof(macKey));

  return DAMO_DUKPT_SUCCESS;
}
#endif

LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Client_PK_Auth_End_Ex(unsigned char *publicKeyFilePath, unsigned char *EncIpekValue, unsigned char *ksn, size_t ksn_len, unsigned char randValueByte[16], unsigned char IpekValue[16])
{
  int ret = DAMO_DUKPT_SUCCESS;
  unsigned char ksn10[10];

  const int error_base = DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_END;
  
  ret = DAMO_DUKPT_Make_KSN10(ksn, ksn_len, ksn10, sizeof(ksn10));
  if ( ret !=  DAMO_DUKPT_SUCCESS )
    return error_base + ret;

  ret = DAMO_DUKPT_Client_PK_Auth_End(publicKeyFilePath, EncIpekValue, ksn10, randValueByte, IpekValue);
  if ( ret !=  DAMO_DUKPT_SUCCESS )
    return ret;

  return DAMO_DUKPT_SUCCESS;
}

int DAMO_DUKPT_Make_KSN10(const unsigned char *ksn, size_t ksn_len, unsigned char *ksn10 /* OUT */, size_t ksn10MaxLen)
{
  int ret = 0;

  if ( ksn == NULL || ksn10 == NULL )
    return DAMO_DUKPT_ERR_CLI_NULL_POINTER;

  if ( ksn_len < 10 )
    return DAMO_DUKPT_ERR_CLI_INVALID_INPUT_DATA;

  if ( ksn10MaxLen < 10 )
    return DAMO_DUKPT_ERR_CLI_INSUFFICIENT_ALLOC_LEN;

  if ( ksn_len == 10 )
    memcpy(ksn10, ksn, ksn_len);
  else
  {
    unsigned char hashData[32];

    ret = DAMO_CRYPT_HASH_SHA256(ksn, ksn_len-3, hashData);
    if ( ret != DAMO_DUKPT_SUCCESS )
      return ret;

    memcpy(ksn10,      hashData,       7);
    memcpy(&ksn10[7], &ksn[ksn_len-3], 3);
  }
  return DAMO_DUKPT_SUCCESS;
}

LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_Client_Make_MacData(int dataType, void * st_enc, unsigned char *outData, int *outDataLen )
{
  unsigned char *encData;
  unsigned char macData[33] = {0,};
  unsigned char *ksn;
  unsigned char *req_mac_key;
  int encDataLen  = 0;
  int lenTemp     = 0;
  int ret          = 0;
  int ksnLen       = 0;
  int macDataLen   = 0;

  if(outDataLen <= 0 || dataType < 0)
  {
    return DAMO_DUKPT_ERR_CLI_INVALID_INPUT_LEN;
  }

  if(outData == NULL || st_enc == NULL)
  {
    return DAMO_DUKPT_ERR_CLI_NULL_POINTER;
  }

	if(dataType == DAMO_DUKPT_CLIENT_MACDATA1)
  {
    encData     = ((DukptClientMacData1*) st_enc) -> encData;
    encDataLen  = (((DukptClientMacData1*) st_enc) -> encDataLen);
    ksn         = ((DukptClientMacData1*) st_enc) -> ksn;
    ksnLen      = ((DukptClientMacData1*) st_enc) -> ksnLen;
    req_mac_key = ((DukptClientMacData1*) st_enc) -> req_mac_key;

    if(ksnLen != 10 || encDataLen < 0)
    {
      return DAMO_DUKPT_ERR_CLI_INVALID_INPUT_LEN;
    }

    /* 1 - ksn */
    memcpy(outData, ksn, ksnLen);
    *outDataLen += ksnLen;

    /* 2 - ksn + encLen */
    lenTemp = htonl(encDataLen);

    memcpy(&outData[*outDataLen], &lenTemp, 4);
    *outDataLen += 4;

    /* 3 - ksn + encLen +encData */
    memcpy(&(outData[*outDataLen]), encData, encDataLen);
    *outDataLen += encDataLen;

    ret = DAMO_CRYPT_HMAC_SHA256 (req_mac_key, 16, outData, *outDataLen, macData);
    if( ret != 0 )
    {
      return ret;
    }

    macDataLen = 32;
    memcpy(&(outData[*outDataLen]), macData, macDataLen);
    *outDataLen += macDataLen;

    if (*outDataLen != (macDataLen + ksnLen + encDataLen + 4))
    {
      return DAMO_DUKPT_ERR_CLI_INVALID_VERIFICATION_DATA_LEN;
    }

    return DAMO_DUKPT_SUCCESS;    
  }
  else 
  {
    return DAMO_DUKPT_ERR_CLI_NOT_SUPPORTED_DATATYPE;
  }
}

LIBSPEC_DUKPTCLIENT
int DAMO_DUKPT_KICC_POS_CLIENT(char* publicKeyFilePath, char* in, int inLen, char* out, int* outLen)
{	
	unsigned char buf[258] = {0,};
	unsigned char testEnc[64] = {0,};
	
	int testInLen = 54;
	unsigned char encBuf[256];
		
	const int error_base = DAMO_DUKPT_ERR_CLI_CODE_BASE_PK_AUTH_START;
	
	int dataLen = 0;
	
	int ret=0;
	size_t encOutLen = 0;
	
		// len + OPTION DATA
		memcpy(out, "0617trcodePOS", 13);
		out += 13;
		// 0100 - client request
		memcpy(out, "0100", 4);
		out += 4;
		// requert value - 00
		memcpy(out, "00", 2);
		out += 2;
		// device num
		memcpy(out, "1111111111", 10);
		out += 10;
		// Merchants num
		memcpy(out, "2222222222", 10);
		out += 10;
		// key value 
		memcpy(out, "0001", 4);
		out += 4;
		// check value ? 
		/**/
		memcpy(out, "33333333333333333333333333333333", 32);
		out += 32;
		/**/
		// code 
		memcpy(out, "261M", 4);
		out += 4;
		// version
		memcpy(out, "0001", 4);
		out += 4;
		// serial num
		memcpy(out, "326131000001", 12);
		out += 12;
		// protocol ver
		memcpy(out, "32", 2);
		out += 2;
		// organization code
		memcpy(out, "1400", 4);
		out += 4;
		// public key version 
		memcpy(out, "pkvr", 4);
		out += 4;
		
		// session key HEX
		// bufPoint = buf;
		
		buf[0] = 0x02;
		buf[1] = 0x56;
		
		memcpy(testEnc, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 64);
		
		ret = DAMO_PKCRYPT_pk_encrypt( publicKeyFilePath, testEnc, testInLen, encBuf, &encOutLen, sizeof(encBuf));
		if ( ret != DAMO_DUKPT_SUCCESS && 256 != encOutLen )
			return error_base - ret;
		
		
		memcpy(&buf[2], encBuf, 256);
		DAMO_DUKPT_ByteToHex(out, &dataLen, buf, 258);
		out += dataLen;
		
		*outLen = dataLen + 105;
				
		return DAMO_KICC_POS_SUCCESS;
}
		
