#ifndef DAMO_PKCRYPT_H
#define DAMO_PKCRYPT_H

#include <stdio.h>

#if defined(EXPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllexport)
#elif defined(IMPORT_DUKPTCRYPT)
#define LIBSPEC_DUKPTCRYPT  __declspec(dllimport)
#else
#define LIBSPEC_DUKPTCRYPT
#endif

#define DAMO_PKCRYPT_ERR_INSUFFICIENT_ALLOC_LEN 201
#define DAMO_PKCRYPT_ERR_INVALID_INPUT          202
#define DAMO_PKCRYPT_ERR_INVALID_INPUT_LEN      203
#define DAMO_PKCRYPT_ERR_PK_FILE_NOT_FOUND      205
#define DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT  206
#define DAMO_PKCRYPT_ERR_ENCRYPT 305
#define DAMO_PKCRYPT_ERR_DECRYPT 306
#define DAMO_PKCRYPT_ERR_SIGN    405
#define DAMO_PKCRYPT_ERR_VERIFY  406

LIBSPEC_DUKPTCRYPT
int DAMO_PKCRYPT_pk_encrypt( 
  char    *pubKeyFilePath, 
  unsigned char    *in, 
  size_t   inLen, 
  unsigned char    *out, 
  size_t  *outLen, 
  int      outMax);

LIBSPEC_DUKPTCRYPT
int DAMO_PKCRYPT_pk_decrypt(
  char    *priKeyFilePath, 
  unsigned char    *in, 
  size_t   inLen, 
  unsigned char    *out, 
  size_t  *outLen, 
  int      outMax);

LIBSPEC_DUKPTCRYPT
int DAMO_PKCRYPT_pk_sign( 
  char    *priKeyFilePath, 
  unsigned char    *hash, 
  size_t   hashLen, 
  unsigned char    *sign, 
  size_t  *signLen);

LIBSPEC_DUKPTCRYPT
int DAMO_PKCRYPT_pk_verify( 
  char    *pubKeyFilePath, 
  unsigned char    *hash, 
  size_t   hashLen, 
  unsigned char    *sign, 
  size_t   signLen);

LIBSPEC_DUKPTCRYPT
int DAMO_PKCRYPT_pk_getModExp(
  char          *pubKeyFilePath,
  unsigned char *modulus,
  size_t        *modulusLen,
  size_t         modulusBufLen,
  unsigned char *exponent,
  size_t        *exponentLen,
  size_t         exponentBufLen);

LIBSPEC_DUKPTCRYPT
int DAMO_PKCRYPT_taSIM_pk_encrypt( 
    unsigned char *pubMod, size_t modLen
  , unsigned char *pubExp, size_t expLen
  , unsigned char *in, size_t inLen
  , unsigned char *out, size_t *outLen, size_t outMax);

LIBSPEC_DUKPTCRYPT
int DAMO_PKCRYPT_taSIM_pk_verify( 
    unsigned char *pubMod, size_t modLen
  , unsigned char *pubExp, size_t expLen
  , unsigned char *hash,   size_t   hashLen
  , unsigned char *sign,   size_t   signLen);


#endif

//#ifndef DAMO_PKCRYPT_H
//#define DAMO_PKCRYPT_H
//
//
//#ifndef POLARSSL_CONFIG_H
//#include "ps_config.h"
//#endif
//#ifndef POLARSSL_PK_H
//#include "ps_pk.h"
//#endif
//
//#define DAMO_PKCRYPT_ERR_PK_FILE_NOT_FOUND 205
//#define DAMO_PKCRYPT_ERR_PK_KEY_INVALID_FORMAT 206
//#define DAMO_PKCRYPT_ERR_ENCRYPT 305
//#define DAMO_PKCRYPT_ERR_DECRYPT 306
//#define DAMO_PKCRYPT_ERR_SIGN 405
//#define DAMO_PKCRYPT_ERR_VERIFY 406
//
//int DAMO_PKCRYPT_pk_encrypt( 
//  char    *pubKeyFilePath, 
//  unsigned char    *in, 
//  size_t   inLen, 
//  unsigned char    *out, 
//  size_t  *outLen, 
//  int      outMax);
//
//int DAMO_PKCRYPT_pk_decrypt(
//  char    *priKeyFilePath, 
//  unsigned char    *in, 
//  size_t   inLen, 
//  unsigned char    *out, 
//  size_t  *outLen, 
//  int      outMax);
//
//int DAMO_PKCRYPT_pk_sign( 
//  char    *priKeyFilePath, 
//  unsigned char    *hash, 
//  size_t   hashLen, 
//  unsigned char    *sign, 
//  size_t  *signLen);
//
//int DAMO_PKCRYPT_pk_verify( 
//  char    *pubKeyFilePath, 
//  unsigned char    *hash, 
//  size_t   hashLen, 
//  unsigned char    *sign, 
//  size_t   signLen);
//
//int DAMO_PKCRYPT_pk_getModExp(
//  char          *pubKeyFilePath,
//  unsigned char *modulus,
//  size_t        *modulusLen,
//  size_t         modulusBufLen,
//  unsigned char *exponent,
//  size_t        *exponentLen,
//  size_t         exponentBufLen);
//
//#endif
