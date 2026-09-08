#include <stdlib.h>
#include <string.h>
#include "entropy.h"
#include "rng.h"

int DAMO_CRYPT_RNG( unsigned char *out, size_t out_len )
{
	int ret;
	HMAC_DRBG_CTX ctx;

	if(!out)
		return DAMO_CRYPT_ERR_RNG_NULL_POINTER;

	ret = DAMO_CRYPT_HMAC_DRBG_Init( &ctx, HMAC_SHA256, DAMO_CRYPT_Get_SHA256_Entropy_Data, NULL, NULL, 0 );
	if( ret != DAMO_CRYPT_SUCCESS )
	{
		return DAMO_CRYPT_ERR_RNG_INTERNAL_INIT_FAIL;
	}
	DAMO_CRYPT_HMAC_DRBG_Set_Prediction_Resistance( &ctx, DAMO_CRYPT_HMAC_DRBG_PR_ON );
	ret = DAMO_CRYPT_HMAC_DRBG_Random_Without_Add( &ctx, out, out_len );
	if( ret != DAMO_CRYPT_SUCCESS )
	{
		return DAMO_CRYPT_ERR_RNG_GEN_RAND_FAIL;
	}
	ret = DAMO_CRYPT_HMAC_DRBG_Random_Without_Add( &ctx, out, out_len );
	if( ret != DAMO_CRYPT_SUCCESS )
	{
		return DAMO_CRYPT_ERR_RNG_GEN_RAND_FAIL;
	}
	DAMO_CRYPT_HMAC_DRBG_Destroy( &ctx );

	return DAMO_CRYPT_SUCCESS;
}
