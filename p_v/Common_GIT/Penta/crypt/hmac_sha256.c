#include <stdlib.h>
#include <string.h>
#include "hmac.h"
#include "hmac_sha256.h"

#define INLENMAX 232

#ifndef DUKPT_SERVER
  static HMAC_SHA256_CTX gctx[1];
#endif
static void* DAMO_CRYPT_HMAC_SHA256_CTX_Create( void )
{
#ifdef DUKPT_SERVER
	HMAC_SHA256_CTX *ctx;
	ctx = (HMAC_SHA256_CTX *) malloc( sizeof( HMAC_SHA256_CTX ) );

	if( ctx == NULL )
		return( NULL );

  return ctx;
#else
	return( gctx );
#endif
}

static void DAMO_CRYPT_HMAC_SHA256_CTX_Destroy( void *ctx )
{
	memset(ctx, 0x00, sizeof( HMAC_SHA256_CTX ));
	
#ifdef DUKPT_SERVER
  free(ctx);
#endif
}

void DAMO_CRYPT_HMAC_SHA256_Init( void *ctx, const unsigned char *key, size_t key_len )
{
	size_t i;
	unsigned char sum[32];
	HMAC_SHA256_CTX *tmp_ctx;

	tmp_ctx = (HMAC_SHA256_CTX *)ctx;

	if( key_len > 64 )
	{
		DAMO_CRYPT_HASH_SHA256( key, key_len, sum );

		key_len = 32;
		key = sum;
	}

	memset( tmp_ctx->ipad, 0x36, 64 );
	memset( tmp_ctx->opad, 0x5C, 64 );

	for( i = 0; i < key_len; i++ )
	{
		tmp_ctx->ipad[i] = (unsigned char)( tmp_ctx->ipad[i] ^ key[i] );
		tmp_ctx->opad[i] = (unsigned char)( tmp_ctx->opad[i] ^ key[i] );
	}

	DAMO_CRYPT_HASH_SHA256_Init(&tmp_ctx->hash_ctx);
	DAMO_CRYPT_HASH_SHA256_Update(&tmp_ctx->hash_ctx, tmp_ctx->ipad, 64);
}

void DAMO_CRYPT_HMAC_SHA256_Update( void *ctx, const unsigned char *data, size_t data_len )
{
	HMAC_SHA256_CTX *tmp_ctx;

	tmp_ctx = (HMAC_SHA256_CTX *)ctx;

	DAMO_CRYPT_HASH_SHA256_Update(&tmp_ctx->hash_ctx, data, data_len);
}

void DAMO_CRYPT_HMAC_SHA256_Final( void *ctx, unsigned char digest[32] )
{
	unsigned char tmp_buf[32];
	HMAC_SHA256_CTX *tmp_ctx;

	tmp_ctx = (HMAC_SHA256_CTX *)ctx;

	DAMO_CRYPT_HASH_SHA256_Final( &tmp_ctx->hash_ctx, tmp_buf );
	DAMO_CRYPT_HASH_SHA256_Init( &tmp_ctx->hash_ctx );
	DAMO_CRYPT_HASH_SHA256_Update( &tmp_ctx->hash_ctx, tmp_ctx->opad, 64 );
	DAMO_CRYPT_HASH_SHA256_Update( &tmp_ctx->hash_ctx, tmp_buf, 32 );
	DAMO_CRYPT_HASH_SHA256_Final( &tmp_ctx->hash_ctx, digest );
}

void DAMO_CRYPT_HMAC_SHA256_Reset( void *ctx )
{
	HMAC_SHA256_CTX *tmp_ctx;

	if(!ctx)
		return;

	tmp_ctx = (HMAC_SHA256_CTX *)ctx;

	DAMO_CRYPT_HASH_SHA256_Init( &tmp_ctx->hash_ctx );
	DAMO_CRYPT_HASH_SHA256_Update( &tmp_ctx->hash_ctx, tmp_ctx->ipad, 64 );
}

int DAMO_CRYPT_HMAC_SHA256(const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len, unsigned char *digest)
{
	HMAC_SHA256_CTX ctx;

	if(!key || !data || !digest)
		return DAMO_CRYPT_ERR_HMAC_SHA256_NULL_POINTER;

	DAMO_CRYPT_HMAC_SHA256_Init( &ctx, key, key_len );
	DAMO_CRYPT_HMAC_SHA256_Update( &ctx, data, data_len );
	DAMO_CRYPT_HMAC_SHA256_Final( &ctx, digest );

	return DAMO_CRYPT_SUCCESS;
}

const HMAC_Desc hmac_sha256 = {
	HMAC_SHA256,
	32,
	DAMO_CRYPT_HMAC_SHA256_Init,
	DAMO_CRYPT_HMAC_SHA256_Update,
	DAMO_CRYPT_HMAC_SHA256_Final,
	DAMO_CRYPT_HMAC_SHA256_Reset,
	DAMO_CRYPT_HMAC_SHA256,
	DAMO_CRYPT_HMAC_SHA256_CTX_Create,
	DAMO_CRYPT_HMAC_SHA256_CTX_Destroy
};
