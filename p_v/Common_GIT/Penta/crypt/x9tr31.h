#ifndef X9TR31_H_
#define X9TR31_H_

#ifdef __cplusplus
extern "C" {
#endif


#define ENCRYPT		1
#define DECRYPT		0

#define DAMO_X9TR31_CREATE_ERROR_KEYTYPE                -200000
#define DAMO_X9TR31_INTERPRETATION_ERROR_MAC_VERIFY     -200100
#define DAMO_X9TR31_INTERPRETATION_ERROR_MSG_LEN        -200101


#if defined(EXPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllexport)
#elif defined(IMPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTCRYPT
#endif

LIBSPEC_DUKPTCRYPT
void DAMO_X9TR31_Get_Enc_Mac_Key(unsigned char *keyBlockProtectionKey, unsigned char *encryptionKey, unsigned char *MACKey);

LIBSPEC_DUKPTCRYPT
void DAMO_X9TR31_XOR(unsigned char *in, int dataSize, unsigned char *out);

LIBSPEC_DUKPTCRYPT
void DAMO_X9TR31_GetSubKey(unsigned char *key, unsigned char *K1, unsigned char *K2, int compatibility);

LIBSPEC_DUKPTCRYPT
void DAMO_X9TR31_AES_GetSubKey(unsigned char *key, unsigned char *K1, unsigned char *K2, int compatibility);

LIBSPEC_DUKPTCRYPT
int DAMO_DUKPT_X9TR31_CreateMsg(unsigned char *keyType, unsigned char *KeyBlockProtectionKey, int KeyBlockProtectionKeyLen, unsigned char *BinaryKeyData, int BinaryKeyDataLen, int padType, unsigned char *msg, int *msgLen);
LIBSPEC_DUKPTCRYPT
int DAMO_DUKPT_X9TR31_InterpretMsg(unsigned char *KeyBlockProtectionKey, int KeyBlockProtectionKeyLen, unsigned char *msg, int msgLen, int padType, unsigned char *BinaryKeyData, int *BinaryKeyDataLen);

LIBSPEC_DUKPTCRYPT
void DAMO_DUKPT_ByteToHex( char *str, int *strLen, 
                unsigned char *in, int inLen );

LIBSPEC_DUKPTCRYPT
int DAMO_DUKPT_HexToByte(unsigned char *out, int *outLen, 
               char *str, int strLen);

#ifdef __cplusplus
}
#endif


#endif /* X9TR31_H_ */
