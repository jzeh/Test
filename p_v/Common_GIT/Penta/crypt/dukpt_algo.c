#include "dukpt_algo.h"
#include "aes.h"
#include "seed.h"
//MONI #include "hight.h"
#include "des.h"

#ifndef DUKPT_SERVER
  #include "dukpt_cli_config.h"
#endif

int DAMO_CRYPT_AES_Encrypt_Core(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, size_t pad);
int DAMO_CRYPT_AES_Decrypt_Core(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, size_t pad);
#if false  //mod.kks 21.12.08 todo remove 
//int DAMO_CRYPT_SEED_Encrypt_Core(unsigned char *out, size_t *outLen, const unsigned char *in, size_t inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad);
//int DAMO_CRYPT_SEED_Decrypt_Core(unsigned char *out, size_t *outLen, const unsigned char *in, size_t inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad);
//int DAMO_CRYPT_HIGHT_Encrypt_Core( OUT unsigned char *out, size_t *outLen, IN const unsigned char *in, IN size_t inLen, IN const unsigned char *key, int mode, IN unsigned char *iv, size_t pad );
//int DAMO_CRYPT_HIGHT_Decrypt_Core(OUT unsigned char *out, size_t *outLen, IN const unsigned char *in, IN size_t inLen, IN const unsigned char *key, int mode, IN unsigned char *iv, size_t pad);
#endif
int DAMO_CRYPT_TDES_Encrypt_Core(unsigned char *out, size_t *outLen, const unsigned char *in, int inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad );
int DAMO_CRYPT_TDES_Decrypt_Core(unsigned char *out, size_t *outLen, const unsigned char *in, int inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad);


#include "MngModem.h"

// MONI binary output
#ifndef USE_KMS_BASE64_ENCRYPT
#define DAMO_DUKPT_ENCRYPT_HEX_OUTPUT
#endif

void DAMO_DUKPT_ByteToHex(char *str, int *strLen, unsigned char *in, int inLen);
int  DAMO_DUKPT_HexToByte(unsigned char *out, int *outLen, char *str, int strLen);


int DAMO_CRYPT_EncryptEx(unsigned char *out, size_t *out_len,
const		unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, size_t pad)
{
  int ret;
  unsigned char in_ivec[16] = {0};

  if (ivec != NULL)
  {
    if(alg == TDES || alg == HIGHT)
    {
      memcpy(in_ivec, ivec, 8);
    }
    else
    {
      memcpy(in_ivec, ivec, key_len);
    }
  }
  
  switch(alg)
  {
      case AES_128:
      case AES_192:
      case AES_256:
	  {
		  ret = DAMO_CRYPT_AES_Encrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, pad);
	  }
	  break;
#if false //mod.kks 21.12.08       
	  case SEED_128:
	  {
		  ret = DAMO_CRYPT_SEED_Encrypt_Core(out, out_len, in, in_len, key, mode, in_ivec, pad);
	  }
	  break;
     
	  case HIGHT:
	  {
		  ret = DAMO_CRYPT_HIGHT_Encrypt_Core(out, out_len, in, in_len, key, mode, in_ivec, pad);
	  }
	  break;
#endif      
	  case TDES:
	  {
		  ret = DAMO_CRYPT_TDES_Encrypt_Core(out, out_len, in, in_len, key, mode, in_ivec, pad);
	  }
	  break;
	  default:
		  return DAMO_CRYPT_ERR_ENC_INVALID_ALG;
  }

#ifdef DAMO_DUKPT_ENCRYPT_HEX_OUTPUT
  {
    size_t enc_len;
    int    outLen = 0;
#if defined(DUKPT_SERVER) || defined(DUKPT_CLIENT_ENCRYPT_ALLOC)
    unsigned char *enc = NULL;
    enc = malloc((*out_len*2)+1);
    if ( enc == NULL )
      return DAMO_CRYPT_ERR_MALLOC;
#else
    unsigned char enc[INLENMAX*2+1];
#endif

    memcpy(enc, out, *out_len);
    enc_len = *out_len;

    DAMO_DUKPT_ByteToHex(out, &outLen, enc, (int)enc_len);
    *out_len = outLen;

#if defined(DUKPT_SERVER) || defined(DUKPT_CLIENT_ENCRYPT_ALLOC)
    if ( enc  )
      free(enc);
#endif
  }
#endif

  return ret;
}

int DAMO_CRYPT_DecryptEx(unsigned char *out, size_t *out_len,
	const	unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, size_t pad)
{
  int ret;
  unsigned char in_ivec[16] = {0};
  unsigned char *inData = (unsigned char *)in;
  size_t         inData_len = in_len;

#ifdef DAMO_DUKPT_ENCRYPT_HEX_OUTPUT
  int    outLen = 0;
#if defined(DUKPT_SERVER) || defined(DUKPT_CLIENT_ENCRYPT_ALLOC)
  unsigned char *enc = NULL;
  enc = malloc(in_len/2+1);

  if ( enc == NULL )
    return DAMO_CRYPT_ERR_MALLOC;
#else
  unsigned char enc[INLENMAX];
#endif

  ret = DAMO_DUKPT_HexToByte(enc, &outLen, inData, (int)in_len);
  if ( ret != 0 )
    return DAMO_CRYPT_ERR_INVALID_HEX_DATA;

  inData = enc;
  inData_len = outLen;
#endif

  if (ivec != NULL)
  {
    if(alg == TDES || alg == HIGHT)
    {
      memcpy(in_ivec, ivec, 8);
    }
    else
    {
      memcpy(in_ivec, ivec, key_len);
    }
  }

  switch(alg)
  {
    case AES_128:
    case AES_192:
    case AES_256:
	  {
	    ret = DAMO_CRYPT_AES_Decrypt_Core(out, out_len, inData, inData_len, key, key_len, alg, mode, in_ivec, pad);
	  }
	  break;
#if false //mod.kks 21.12.08 todo remove       
	  case SEED_128:
	  {
		ret = DAMO_CRYPT_SEED_Decrypt_Core(out, out_len, inData, inData_len, key, mode, in_ivec, pad);
	  }
	  break;
	  case HIGHT:
	  {
        ret = DAMO_CRYPT_HIGHT_Decrypt_Core(out, out_len, inData, inData_len, key, mode, in_ivec, pad);
	  }
	  break;
#endif      
	  case TDES:
	  {
	    ret = DAMO_CRYPT_TDES_Decrypt_Core(out, out_len, inData, inData_len, key, mode, in_ivec, pad);
	  }
	  break;
	  default:
#ifdef DAMO_DUKPT_ENCRYPT_HEX_OUTPUT
#if defined(DUKPT_SERVER) || defined(DUKPT_CLIENT_ENCRYPT_ALLOC)
      if ( enc  )
        free(enc);
#endif
#endif
		  return DAMO_CRYPT_ERR_DEC_INVALID_ALG;
  }  

#ifdef DAMO_DUKPT_ENCRYPT_HEX_OUTPUT
#if defined(DUKPT_SERVER) || defined(DUKPT_CLIENT_ENCRYPT_ALLOC)
  if ( enc  )
    free(enc);
#endif
#endif

  return ret;
}

