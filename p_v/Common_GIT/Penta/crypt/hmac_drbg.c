#include <stdlib.h>
#include <string.h>
#include "hmac_drbg.h"

void DAMO_CRYPT_HMAC_DRBG_Update( HMAC_DRBG_CTX *ctx, const unsigned char *add_data, size_t add_data_len )
{
    size_t digest_len;
    unsigned char round_num = 1;
    unsigned char index;
    unsigned char K[DAMO_CRYPT_DIGEST_MAX_SIZE];

	digest_len = DAMO_CRYPT_HMAC_GET_DIGEST_LEN(ctx->hmac_ctx.hmac_type);

	if( add_data != NULL && add_data_len != 0 )
		round_num = 2;

    for( index = 0; index < round_num; index++ )
    {
        DAMO_CRYPT_HMAC_Reset( &ctx->hmac_ctx );
        DAMO_CRYPT_HMAC_Update( &ctx->hmac_ctx, ctx->V, digest_len );
        DAMO_CRYPT_HMAC_Update( &ctx->hmac_ctx, &index, 1 );
        if( round_num == 2 )
            DAMO_CRYPT_HMAC_Update( &ctx->hmac_ctx, add_data, add_data_len );
        DAMO_CRYPT_HMAC_Final( &ctx->hmac_ctx, K, &digest_len );

        DAMO_CRYPT_HMAC_Init( &ctx->hmac_ctx, K, digest_len );
        DAMO_CRYPT_HMAC_Update( &ctx->hmac_ctx, ctx->V, digest_len );
        DAMO_CRYPT_HMAC_Final( &ctx->hmac_ctx, ctx->V, &digest_len );
    }
}

int DAMO_CRYPT_HMAC_DRBG_Reseed( HMAC_DRBG_CTX *ctx, const unsigned char *add_data, size_t add_data_len )
{
    unsigned char seed[DAMO_CRYPT_HMAC_DRBG_MAX_SEED_BYTES];
    size_t seed_len;

    /* 입력길이 체크 */
    if( add_data_len > DAMO_CRYPT_HMAC_DRBG_MAX_INPUT_BYTES ||
        ctx->entropy_len + add_data_len > DAMO_CRYPT_HMAC_DRBG_MAX_SEED_BYTES )
    {
        return DAMO_CRYPT_ERR_HMAC_DRBG_INPUT_TOO_BIG;
    }

    memset( seed, 0, DAMO_CRYPT_HMAC_DRBG_MAX_SEED_BYTES );

    /* entropy_len 바이트만큼의 Seed에 대한 엔트로피 수집 */
    if( ctx->entropy_func( ctx->entropy_ctx, seed, ctx->entropy_len ) != DAMO_CRYPT_SUCCESS )
        return DAMO_CRYPT_ERR_HMAC_DRBG_ENTROPY_SOURCE_FAILED;

    seed_len = ctx->entropy_len;

    /* 엔트로피와 추가적인 데이터를 연접 */
    if( add_data != NULL && add_data_len != 0 )
    {
        memcpy( seed + seed_len, add_data, add_data_len );
        seed_len += add_data_len;
    }

    /* State 업데이트 */
    DAMO_CRYPT_HMAC_DRBG_Update( ctx, seed, seed_len );

    /* Reseed Counter 재설정 */
    ctx->reseed_counter = 1;

	return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_HMAC_DRBG_Init( HMAC_DRBG_CTX *ctx, HMAC_TYPE hmac_type,
		int (*entropy_func)(void *, unsigned char *, size_t), void *entropy_ctx,
		const unsigned char *add_data, size_t add_data_len )
{
    int ret;
    size_t entropy_len;
	int digest_len;

    memset( ctx, 0, sizeof( HMAC_DRBG_CTX ) );

	memset( &ctx->hmac_ctx, 0, sizeof(HMAC_CTX));
	ret = DAMO_CRYPT_HMAC_CTX_Create( &ctx->hmac_ctx, hmac_type );
	if( ret != DAMO_CRYPT_SUCCESS ) return ret;

	digest_len = DAMO_CRYPT_HMAC_GET_DIGEST_LEN(hmac_type);

	DAMO_CRYPT_HMAC_Init( &ctx->hmac_ctx, ctx->V, digest_len );
    memset( ctx->V, 0x01, digest_len );

    ctx->entropy_func = entropy_func;
    ctx->entropy_ctx = entropy_ctx;

    ctx->reseed_interval = 10000;

	if(digest_len <= 20) /* 60비트 이하 해쉬 => 128비트 */
		entropy_len = 16;
	else if(digest_len <= 28) /* 224비트 이하 해쉬 => 192비트 */
		entropy_len = 24;
	else /* 256비트 이상 해쉬 => 256비트 */
		entropy_len = 32;

    ctx->entropy_len = entropy_len * 3 / 2;

	ret = DAMO_CRYPT_HMAC_DRBG_Reseed( ctx, add_data, add_data_len );
	if( ret != DAMO_CRYPT_SUCCESS ) return ret;

    ctx->entropy_len = entropy_len;

    return DAMO_CRYPT_SUCCESS;
}

void DAMO_CRYPT_HMAC_DRBG_Destroy( HMAC_DRBG_CTX *ctx )
{
	if( ctx == NULL )
		return;

	DAMO_CRYPT_HMAC_CTX_Destroy( &ctx->hmac_ctx );
}

int DAMO_CRYPT_HMAC_DRBG_Random_With_Add( HMAC_DRBG_CTX *ctx, 
		unsigned char *output, size_t output_len, 
		const unsigned char *add_data, size_t add_data_len )
{
    int ret;
    size_t digest_len;
	size_t use_len;
    size_t left = output_len;
    unsigned char *out = output;

	digest_len = DAMO_CRYPT_HMAC_GET_DIGEST_LEN(ctx->hmac_ctx.hmac_type);

    /* Request 길이 체크 */
    if( output_len > DAMO_CRYPT_HMAC_DRBG_MAX_REQUEST )
        return( DAMO_CRYPT_ERR_HMAC_DRBG_REQUEST_TOO_BIG );

    /* 입력 길이 체크 */
    if( add_data_len > DAMO_CRYPT_HMAC_DRBG_MAX_INPUT_BYTES )
        return( DAMO_CRYPT_ERR_HMAC_DRBG_INPUT_TOO_BIG );

    /* Reseed Counter, Prediction_Resistance 체크  */
    if( ctx->entropy_func != NULL &&
			( ctx->prediction_resistance == DAMO_CRYPT_HMAC_DRBG_PR_ON ||
			  ctx->reseed_counter > ctx->reseed_interval ) )
    {
		ret = DAMO_CRYPT_HMAC_DRBG_Reseed( ctx, add_data, add_data_len );
		if( ret != DAMO_CRYPT_SUCCESS ) return ret;

        add_data_len = 0;
    }

    /* 추가적인 데이터가 존재할시 처리 */
    if( add_data != NULL && add_data_len != 0 )
        DAMO_CRYPT_HMAC_DRBG_Update( ctx, add_data, add_data_len );

    /* 랜덤 데이터 생성 */
    while( left != 0 )
    {
        use_len = left > digest_len ? digest_len : left;

        DAMO_CRYPT_HMAC_Reset( &ctx->hmac_ctx );
        DAMO_CRYPT_HMAC_Update( &ctx->hmac_ctx, ctx->V, digest_len );
        DAMO_CRYPT_HMAC_Final( &ctx->hmac_ctx, ctx->V, &digest_len );

        memcpy( out, ctx->V, use_len );
        out += use_len;
        left -= use_len;
    }

    /* 추가적인 데이터 HMAC 업데이트 */
    DAMO_CRYPT_HMAC_DRBG_Update( ctx, add_data, add_data_len );

    /* Reseed Counter 업데이트 */
    ctx->reseed_counter++;

	return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_HMAC_DRBG_Random_Without_Add( HMAC_DRBG_CTX *ctx, unsigned char *output, size_t output_len )
{
    return DAMO_CRYPT_HMAC_DRBG_Random_With_Add( ctx, output, output_len, NULL, 0 );
}

void DAMO_CRYPT_HMAC_DRBG_Set_Prediction_Resistance( HMAC_DRBG_CTX *ctx, int resistance )
{
	ctx->prediction_resistance = resistance;
}

void DAMO_CRYPT_HMAC_DRBG_Set_Entropy_Len( HMAC_DRBG_CTX *ctx, size_t len )
{
	ctx->entropy_len = len;
}

void DAMO_CRYPT_HMAC_DRBG_Set_Reseed_Interval( HMAC_DRBG_CTX *ctx, int interval )
{
	ctx->reseed_interval = interval;
}
