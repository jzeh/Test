#ifndef	__GIT_HSM_H__
#define	__GIT_HSM_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "stream_buffer.h"
#include "typedef.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define ISO7816_OK											( 0)
#define ISO7816_ERR											(-1)
#define ISO7816_ERR_TIMEPOUT								(-2)
#define ISO7816_ERR_INIT_PWM								(-3)
#define ISO7816_ERR_INIT_GPIO								(-4)
#define ISO7816_ERR_INIT_ISO7816							(-5)
#define ISO7816_ERR_ATR_MUTE								(-6)
#define ISO7816_ERR_ATR_WRONG								(-7)

#define INTERNAL_MASTER_KEY									( 0)
#define INTERNAL_AUTH_KEY									( 1)
#define INTERNAL_HIKEY										( 2)

#define	SIZE_GIT_FILES			600
#define	INDEX_GIT_FILES			7	//(1~7)

// Error Code
#define	HSM_SUCCESS											200
#define	HSM_UNKNOWN_ERROR									300
#define	HSM_INVALID_REQUEST									400
#define	HSM_INVALID_KEYLENGTH								401
#define	HSM_CHECKSUM_ERROR									402
#define	HSM_INSUFFICIENT_CAPA								507
#define	HSM_NOT_RESPONSE									602
#define	HSM_NO_AUTHKEY_INDEX								619
#define	HSM_NEDD_INTERNAL_AUTH								620
#define	HSM_FAIL_EXTERNAL_AUTH								621
#define	HSM_NEED_MUTUAL_AUTH								623

#define	SIZE_DATA_RSA1024									128
#define	SIZE_DATA_RSA2048									256
#define	SIZE_PRIVATEKEY_RSA2048								(256+256+256)
#define	SIZE_GIT_CERTIFICATE								600 //608

#define HSM_CERTI_VER_LEN 		1
#define HSM_INVALID_DATE_LEN 	6
#define HSM_HOLDER_REF_LEN 		16

#define	HSM_NORMAL_MODE										0x00
#define	HSM_UPDATE_MODE										0x01
#define	HSM_SEND_HSM_DATA									0x02

#define AES_INDEX_1                                         0x00 //For DSSAD
#define AES_INDEX_2                                         0x01
#define AES_INDEX_3                                         0x02
#define AES_INDEX_4                                         0x03
#define AES_INDEX_5                                         0x04
#define AES_INDEX_6                                         0x05 //For ECU CODE

#define HMAC_INDEX_1                                        0x00 //For DSSAD MAC
#define HMAC_INDEX_2                                        0x01
#define HMAC_INDEX_3                                        0x02
#define HMAC_INDEX_4                                        0x03
   
#define AES_ECB                                             0x00
#define AES_CBC                                             0x01
#define AES_CTR                                             0x02

//Use with ReadPublicKeyHSM
#define HSM_PUBKEY                                          0x00 //This Public Key is made by HSM

/* NEW HSM Key Slot Definitions are in git_HSM_Operations.h */

/* ============================================
 * Mutual Auth Buffer Sizes (OLD vs NEW HSM)
 * ============================================ */
/* OLD HSM (APDU) - HMAC-MD5 based */
#define HSM_OLD_RANDOM_SIZE                                 8    // SR, CR = 8 bytes
#define HSM_OLD_SIGN_SIZE                                   16   // Sign1, Sign2 = 16 bytes

/* NEW HSM (SPI) - HMAC-SHA256 based */
#define HSM_NEW_RANDOM_SIZE                                 16   // SR, CR = 16 bytes
#define HSM_NEW_SIGN_SIZE                                   32   // Sign1, Sign2 = 32 bytes

#define PKEY1_PUBKEY                                        0x01 //Public Key of P_SHA1_Key
#define PKEY2_PUBKEY                                        0x02 //Public Key of P_SHA2_Key
#define PKEY3_PUBKEY                                        0x03 //Public Key of C_SHA1_Key
#define PKEY4_PUBKEY                                        0x04 //Public Key of C_SHA2_Key
#define PKEY5_PUBKEY                                        0x05 //Public Key of Reserved1_Key
#define PKEY6_PUBKEY                                        0x06 //Public Key of Reserved2_Key
#define PKEY7_PUBKEY                                        0x07 //Public Key of Reserved3_Key
   
//Use with StorePrivateKeyHSM, StoreCertificateHSM
#define PA_SHA1                                             0x01 // For passenger car 
#define PA_SHA2                                             0x02 // For passenger car 
#define CO_SHA1                                             0x03 // For Commercial car 
#define CO_SHA2                                             0x04 // For Commercial car 
#define RESERVED_1                                          0x05 // Reseved
#define RESERVED_2                                          0x06 // Reseved
#define RESERVED_3                                          0x07 // Reseved

#define CRL_LENGTH                                          501

#define AES128                                              0
#define AES256                                              1
   
#define ASK_MASTER                                              0
#define ASK_REWORK                                              1

#define ASK_DIAG_PV                                         0
#define ASK_REWORK_PV                                       1
#define ASK_DIAG_CV                                         2
#define ASK_REWORK_CV                                       3

#define HSM_ATR_LEN											34

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
typedef struct
{
	uint8_t		mProfile;									// 인증서 버전
	uint8_t		mAuthRefer[14];								// 인증기관
	uint8_t		mEffectDate[3];								// 인증서 발생일
	uint8_t		mExpirDate[3];								// 인증서 만료일
	uint8_t		mAuthTemplate[23];							// 인증기관 ID
	uint8_t		mHolderRefer[20];							// 인증서 소유자 정보
	uint8_t		mPublicKeyID[20];							// 공개키 OID
	uint8_t		mPublicKeyEx[4];							// 공개키 Exponent
	uint8_t		mPublicKeyMo[256];							// 공개키 Modulus
	uint8_t		mCertSign[256];								// 인증서 전자서명
} Certificate_t;

typedef struct ppsreq_t
{
	uint32_t	cycles_per_etu;
	uint8_t		cmd[4];
} st_PPSReq;

typedef __packed struct _SHSMUpdateAck
{
	uint32_t	mProgress;
    bool		mResult;
	bool	    mSelftestMode;
} SHSMUpdateAck;

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int8_t	InitHSM( void );													// HSM init
extern void		DeinitHSM( void );													// HSM deinit
extern void		ResetHSM( uint8_t rst );											// HSM reset
extern void		ColdResetHSM( uint8_t rst );										// HSM cold reset
extern void		transmitHSMCommand( uint8_t *data, uint8_t len );
extern void		clearHSMReceiveData( void );
extern uint32_t	getHSMReceiveDataSize( void );
extern uint32_t	getHSMReceiveData( uint8_t *data, uint32_t len );

extern void Crypto_Mutex_Init(void);   /* create the crypto serialization mutex; call once at startup before any crypto use */
extern int32_t setRSA_Encrypt(uint8_t *pInMessage, uint32_t MsgLength, uint8_t *pKeyString, uint32_t KeySize, uint8_t *pOutMessage);
extern int32_t setRSA_Decrypt(uint8_t *pInMessage, uint32_t *MsgLength, uint8_t *pKeyMo,uint8_t *pKeyEx, uint32_t KeyMoSize, uint32_t KeyExSize, uint8_t *pOutMessage);
extern int32_t setRSA_Verify_SHA1(uint8_t *pInMessage, uint32_t MsgLength, Certificate_t *pKeyString, uint8_t *pSignature);
extern int32_t getAESEncoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t AES_Type, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength );
extern int32_t getAESDecoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t AES_Type, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength );
//extern int32_t RSA_Sign_SHA1(uint8_t *Seed, uint32_t SeedLength, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength ); /* Remove annotations and use if necessary (Q_hyek) */
extern int32_t getTDESEncoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength );
extern int32_t getTDESEncoding_CBC(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *IV, uint32_t IVLength, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength );
extern int32_t getDESEncoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength );
extern int32_t getDESDecoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength );

extern int32_t	se_open( void );				//HSM POWER ON, RESET, READ ATR
extern int32_t	se_close( void );				//POWER OFF
extern int32_t	se_tranreceive( uint8_t *apdu, uint32_t apdulen, uint8_t *resp, uint32_t *resplen );

extern uint32_t read_bytes( uint8_t *buf, uint32_t len );

/*----------------------------------------------------------------------
 *   GIT API Functions(Library API)
 *--------------------------------------------------------------------*/
extern int ActivationHSM(void);
extern int DeactivationHSM(void);
extern int ReadCSNHSM(uint8_t *CSN);
extern int InternalAuthHSM(int KeyId, uint8_t *SR, uint8_t *CR, uint8_t *Sign1);
extern int ExternalAuthHSM(int KeyId, uint8_t *Sign2);
extern int ReadPublicKeyHSM(uint8_t *KpuHSM, int *KeyLen, uint8_t key_num);
extern int StoreAuthKeyHSM(uint8_t *EAuthKey, int KeyLen);
extern int StoreHIKeyHSM(uint8_t *EHIKey, int KeyLen);
extern int TransTempKeyHSM(uint8_t *ETempKey, int KeyLe);
extern int StorePrivateKeyHSM(uint8_t *EPrivateKey, int KeyLen, uint8_t key_num);
extern int StoreCertificateHSM(uint8_t *Certificate, int CertLen, uint8_t Certi_num);
extern int ReadCertificateHSM(uint8_t *Certificate, int *CertLen, uint8_t Certi_num);
extern int SignPrivateHSM(uint8_t *Seed, int SeedLen, uint8_t *Sign, int *SignLen, uint8_t key_num);
extern int SignPrivateHSMSHA256(uint8_t *Seed, int SeedLen, uint8_t *Sign, int *SignLen, uint8_t key_num);
extern int WriteDataHSM(uint8_t *Data, int DataLen, uint8_t File_num);							//index는 0~4까지이며 0부터 사용해도 무방함. 상호인증 필요
extern int ReadDataHSM(uint8_t *Data, int DataLen, uint8_t File_num);
extern int DeleteDataHSM(int DataLen, uint8_t File_num);
extern int StoreAES128HSM(uint8_t *EKey, int KeyLen, uint8_t index, uint8_t *Checksum);
extern int StoreAES128KeyHSMforECUcode(uint8_t *EKey, int KeyLen, uint8_t *Checksum);
extern int ReadAES128KeyChecksumHSM(uint8_t index, uint8_t *Checksum);
extern int DecryptAES128HSM(uint8_t *EData, uint8_t *PData, uint8_t mode, uint8_t Keyindex, uint16_t DataLen);
extern int StoreHMACSHA256KeyHSM(uint8_t *EKey, int KeyLen, uint8_t index);
extern int HMACSHA256SignHSM(uint8_t *Data, uint8_t *EData, uint8_t Keyindex, uint16_t DataLen);
extern int ReadVersionHSM(uint8_t *NumVersion);
extern int UpdateOSHSM(uint32_t APDU_blocknum);
extern int BackToLoadHSM(uint8_t *ValATR);
extern int GenerateRSAKeypairHSM(void);
extern int ReadPublicKeyHSM2(uint8_t *Data, uint32_t *Keylen);
extern int DecryptPrivateKeyHSM(uint8_t *EncryptedData, uint8_t *DecryptedData);
extern int DeleteRSAKeypairHSM(void);
extern int UpdateAppletHSM(uint8_t APDU_blocknum);
extern int DeleteAppletHSM(void);
extern int ASKAppletDeleteHSM(void);
extern int ASKAppletLoadHSM_AID90(uint8_t APDU_blocknum);
extern int ASKAppletLoadHSM_AID92(uint8_t APDU_blocknum);
extern int ASKAppletLoadHSM_AID93(uint8_t APDU_blocknum);
extern int ASKAppletLoadHSM_AID94(uint8_t APDU_blocknum);
extern int ASKSignHSM_AID90(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign);
extern int ASKSignHSM_AID92(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign);
extern int ASKSignHSM_AID93(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign);
extern int ASKSignHSM_AID94(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign);	

extern int32_t InitHSMUart( uint32_t baudrate );

extern int8_t HSM_File_SizeInfo( int *FileSize );
extern int8_t HSM_File_VersionInfo( void );
extern int16_t HSM_Version_Check( int *version );
extern int32_t VCI3_HSM_UPDATE( uint8_t *AuthKey );
extern int32_t HSM_ASK_Applet_Update( uint8_t *AuthKey, bool UpdateObj, uint8_t ASKType );

extern void Save_HSM_Status( void );
extern uint32_t Read_HSM_Status( void );
extern void Save_HSM_Error_Status( U32 ErrorCode );
extern void Save_HSM_UpdateFailCount( void );
extern uint8_t Check_HSM_UpdateFailCount( void );

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern StreamBufferHandle_t	hSBHsmRx;
extern uint8_t				gucHSMRxDummy;

#endif // __GIT_HSM_H__