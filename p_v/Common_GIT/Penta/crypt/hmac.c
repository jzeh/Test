#include <stdlib.h>
#include <string.h>
#include "hmac.h"


extern HMAC_Desc hmac_sha256;

HMAC_Desc *hmac_desc_list[HMAC_TYPE_END]={ &hmac_sha256, };

int DAMO_CRYPT_HMAC_CTX_Create( HMAC_CTX *ctx, HMAC_TYPE hmac_type )
{
	if(!ctx) return DAMO_CRYPT_ERR_HMAC_NULL_POINTER;

	if(hmac_type >= HMAC_TYPE_END) return DAMO_CRYPT_ERR_HMAC_NULL_POINTER;

	ctx->hmac_type = hmac_type;
	ctx->ctx = hmac_desc_list[hmac_type]->hmac_ctx_create_func();

	return DAMO_CRYPT_SUCCESS;
}

void DAMO_CRYPT_HMAC_CTX_Destroy( HMAC_CTX *ctx )
{
	if(!ctx) return; 

	if(ctx->hmac_type >= HMAC_TYPE_END)
	{
		if(ctx->ctx)
      return;
#ifdef DUKPT_SERVER
			free(ctx->ctx);
#endif
	}
	else
	{
		hmac_desc_list[ctx->hmac_type]->hmac_ctx_destroy_func(ctx->ctx);
	}
}

int DAMO_CRYPT_HMAC_Init( HMAC_CTX *ctx, const unsigned char *key, size_t key_len )
{
	if(!ctx || !ctx->ctx || !key)
		return DAMO_CRYPT_ERR_HMAC_NULL_POINTER;

	hmac_desc_list[ctx->hmac_type]->hmac_init_func( ctx->ctx, key, key_len );

	return DAMO_CRYPT_SUCCESS; 
}

int DAMO_CRYPT_HMAC_Update( HMAC_CTX *ctx, const unsigned char *data, size_t data_len )
{
	if(!ctx || !ctx->ctx || !data)
		return DAMO_CRYPT_ERR_HMAC_NULL_POINTER;

	hmac_desc_list[ctx->hmac_type]->hmac_update_func( ctx->ctx, data, data_len );

	return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_HMAC_Final( HMAC_CTX *ctx, unsigned char *digest, size_t *digest_len )
{
	if(!ctx || !ctx->ctx || ! digest)
		return DAMO_CRYPT_ERR_HMAC_NULL_POINTER;

	hmac_desc_list[ctx->hmac_type]->hmac_final_func( ctx->ctx, digest );
	(*digest_len) = hmac_desc_list[ctx->hmac_type]->digest_len;

	return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_HMAC_Reset( HMAC_CTX *ctx )
{
	if(!ctx || !ctx->ctx)
		return DAMO_CRYPT_ERR_HMAC_NULL_POINTER;

	hmac_desc_list[ctx->hmac_type]->hmac_reset_func( ctx->ctx );

	return DAMO_CRYPT_SUCCESS;

}

int DAMO_CRYPT_HMAC(HMAC_TYPE hmac_type, const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len, void *digest, size_t *digest_len)
{
	int ret;

	if(hmac_type >= HMAC_TYPE_END) return DAMO_CRYPT_ERR_HMAC_NULL_POINTER;

	ret = hmac_desc_list[hmac_type]->hmac_func( key, key_len, data, data_len, digest );
	(*digest_len) = hmac_desc_list[hmac_type]->digest_len;

	return ret;
}

int DAMO_CRYPT_HMAC_GET_DIGEST_LEN(HMAC_TYPE hmac_type)
{
	return hmac_desc_list[hmac_type]->digest_len;
}
