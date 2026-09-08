#include "damo_pkcrypt.h"

#ifndef POLARSSL_CONFIG_H
#include "ps_config.h"
#endif
#ifndef POLARSSL_PK_H
#include "ps_pk.h"
#endif

int DAMO_PKCRYPT_pk_encrypt( char* pubKeyFilePath, unsigned char* in, size_t inLen, unsigned char* out, size_t* outLen, int outMax)
{
  pk_context pk;
  int ret = -1;

  pk_init(&pk);

  ret = pk_parse_public_keyfile(&pk, pubKeyFilePath);
  if ( ret == POLARSSL_ERR_PK_FILE_IO_ERROR)
  {
    pk_free(&pk);
    return DAMO_PKCRYPT_ERR_PK_FILE_NOT_FOUND;
  }
  else if ( ret == POLARSSL_ERR_PK_KEY_INVALID_FORMAT)
  {
    pk_free(&pk);
    return DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT;
  }

  ret = pk_encrypt( &pk, in,inLen, out, outLen, outMax);
  
  if( ret != 0)
  {
    pk_free(&pk);
    return DAMO_PKCRYPT_ERR_ENCRYPT;
  }

  pk_free(&pk);
  return ret;
}

int DAMO_PKCRYPT_pk_decrypt( char* priKeyFilePath, unsigned char* in, size_t inLen, unsigned char* out, size_t* outLen, int outMax)
{
  pk_context vk;
  int ret = -1;

  pk_init(&vk);

  ret = pk_parse_keyfile(&vk, priKeyFilePath, "");
  if ( ret == POLARSSL_ERR_PK_FILE_IO_ERROR)
  {
    pk_free(&vk);
    return DAMO_PKCRYPT_ERR_PK_FILE_NOT_FOUND;
  }
  else if ( ret == POLARSSL_ERR_PK_KEY_INVALID_FORMAT)
  {
    pk_free(&vk);
    return DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT;
  }
  else if ( ret != 0)
  {
    pk_free(&vk);
    return ret;
  }

  ret = pk_decrypt( &vk, in, inLen, out, outLen, outMax);
  
  if ( ret != 0)
  {
    pk_free(&vk);
    return DAMO_PKCRYPT_ERR_DECRYPT;
  }

  pk_free(&vk);
  return ret;
}

int DAMO_PKCRYPT_pk_sign( char* priKeyFilePath, unsigned char* hash, size_t hashLen, unsigned char* sign, size_t* signLen)
{
  pk_context vk;
  int ret = -1;

  pk_init(&vk);
  
  ret = pk_parse_keyfile(&vk, priKeyFilePath, "");
  if ( ret == POLARSSL_ERR_PK_FILE_IO_ERROR)
  {
    pk_free(&vk);
    return DAMO_PKCRYPT_ERR_PK_FILE_NOT_FOUND;
  }
  else if ( ret == POLARSSL_ERR_PK_KEY_INVALID_FORMAT)
  {
    pk_free(&vk);
    return DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT;
  }
  else if ( ret != 0)
  {
    pk_free(&vk);
    return ret;
  }
    

  ret = pk_sign( &vk, POLARSSL_MD_SHA256, hash, hashLen, sign, signLen);

  if ( ret != 0)
  {
    pk_free(&vk);
    return DAMO_PKCRYPT_ERR_SIGN;
  }

  pk_free(&vk);
  return ret;
}

int DAMO_PKCRYPT_pk_verify( char* pubKeyFilePath, unsigned char* hash, size_t hashLen, unsigned char* sign, size_t signLen)
{
  pk_context pk;
  int ret = -1;

   pk_init(&pk);

   ret = pk_parse_public_keyfile(&pk, pubKeyFilePath);
   if ( ret == POLARSSL_ERR_PK_FILE_IO_ERROR)
   {
     pk_free(&pk);
     return DAMO_PKCRYPT_ERR_PK_FILE_NOT_FOUND;
   }
   else if ( ret == POLARSSL_ERR_PK_KEY_INVALID_FORMAT)
   {
     pk_free(&pk);
     return DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT;
   }

   ret = pk_verify( &pk, POLARSSL_MD_SHA256, hash, hashLen, sign, signLen);

   if ( ret != 0)
   {
     pk_free(&pk);
     return DAMO_PKCRYPT_ERR_VERIFY;
   }

   pk_free(&pk);
   return ret;
}

int DAMO_PKCRYPT_pk_getModExp(
    char* pubKeyFilePath
  , unsigned char *modulus, size_t *modulusLen, size_t modulusBufLen
  , unsigned char *exponent, size_t *exponentLen, size_t exponentBufLen
)
{
  pk_context pk;
  int ret = -1;
  mpi *m, *e;


  pk_init(&pk);

  ret = pk_parse_public_keyfile(&pk, pubKeyFilePath);
  if ( ret == POLARSSL_ERR_PK_FILE_IO_ERROR)
  {
    pk_free(&pk);
    return DAMO_PKCRYPT_ERR_PK_FILE_NOT_FOUND;
  }
  else if ( ret == POLARSSL_ERR_PK_KEY_INVALID_FORMAT)
  {
    pk_free(&pk);
    return DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT;
  }

  m = (mpi *)&(((rsa_context *)pk.pk_ctx)->N);
  *modulusLen = mpi_size(m);
  if ( modulusBufLen < (*modulusLen) )
  {
    pk_free(&pk);
    return DAMO_PKCRYPT_ERR_INSUFFICIENT_ALLOC_LEN;
  }
  ret = mpi_write_binary(m, modulus, *modulusLen);
  if ( ret != 0 )
  {
    pk_free(&pk);
    return ret;
  }

  e = (mpi *)&(((rsa_context *)pk.pk_ctx)->E);
  *exponentLen = mpi_size(e);
  if ( exponentBufLen < (*exponentLen) )
  {
    pk_free(&pk);
    return DAMO_PKCRYPT_ERR_INSUFFICIENT_ALLOC_LEN;
  }
  ret = mpi_write_binary(e, exponent, *exponentLen);
  if ( ret != 0 )
  {
    pk_free(&pk);
    return ret;
  }
  
  pk_free(&pk);
  return 0;
}

int DAMO_PKCRYPT_taSIM_setModExp(
    unsigned char *pubMod, size_t modLen
  , unsigned char *pubExp, size_t expLen
  , rsa_context *ctx
  , const char *pubKeyFilePath
)
{

  if ( ctx != NULL )
  {
    rsa_init(ctx, RSA_PKCS_V15, 0);

    if ( modLen != 256 )
    {
      return DAMO_PKCRYPT_ERR_INVALID_INPUT_LEN;
    }
    mpi_read_binary(&(ctx->N), pubMod, modLen);
    ctx->len = mpi_size(&(ctx->N));

    if ( expLen > 0 )
    {
      if ( pubExp[0] == 0x03 )
        expLen = 1;
      else if ( pubExp[0] == 0x01 && pubExp[1] == 0x00 && pubExp[2] == 0x01 )
        expLen = 3;
      else
      {
        return DAMO_PKCRYPT_ERR_INVALID_INPUT;
      }
    }
    else
    {
      return DAMO_PKCRYPT_ERR_INVALID_INPUT_LEN;
    }
    mpi_read_binary(&(ctx->E), pubExp, expLen);

    if ( rsa_check_pubkey(ctx) != 0 )
    {
      return DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT;
    }
  }
  else
    return DAMO_PKCRYPT_ERR_INVALID_INPUT;

  if (pubKeyFilePath)
  {
    return DAMO_PKCRYPT_ERR_INVALID_INPUT;
  }

  return 0;
}

int DAMO_PKCRYPT_taSIM_pk_encrypt( 
    unsigned char *pubMod, size_t modLen
  , unsigned char *pubExp, size_t expLen
  , unsigned char *in, size_t inLen
  , unsigned char *out, size_t *outLen, size_t outMax)
{
  int ret = -1;
  rsa_context ctx;

  ret = DAMO_PKCRYPT_taSIM_setModExp(pubMod, modLen, pubExp, expLen, &ctx, NULL);
  if ( ret != 0 )
  {
    rsa_free(&ctx);
    return ret;
  }
  *outLen = ctx.len;

  ret = rsa_rsaes_pkcs1_v15_encrypt(&ctx, RSA_PUBLIC, inLen, in, out);
  rsa_free(&ctx);

  if ( ret != 0)
  {
    return DAMO_PKCRYPT_ERR_ENCRYPT;
  }

  return 0;
}

int DAMO_PKCRYPT_taSIM_pk_verify( 
    unsigned char *pubMod, size_t modLen
  , unsigned char *pubExp, size_t expLen
  , unsigned char *hash,   size_t hashLen
  , unsigned char *sign,   size_t signLen)
{
  int ret = -1;
  rsa_context ctx;

  ret = DAMO_PKCRYPT_taSIM_setModExp(pubMod, modLen, pubExp, expLen, &ctx, NULL);
  if ( ret != 0 )
  {
    rsa_free(&ctx);
    return ret;
  }

  ret = rsa_rsassa_pkcs1_v15_verify(&ctx
    , RSA_PUBLIC, POLARSSL_MD_SHA256
    , hashLen, hash, sign );
  rsa_free(&ctx);

  if ( ret != 0)
  {
    return DAMO_PKCRYPT_ERR_VERIFY;
  }

  return 0;
}

