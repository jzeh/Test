#include <stdlib.h>
#include "aes.h"
#include "aes_tables.h"

#ifdef DUKPT_SERVER
	#include "dukpt_server_module.h"
#else
  #include "dukpt.h"
  #include "dukpt_cli_config.h"
#endif

/* 32-bit Integer Manipulation Macros (Little Endian) */
#ifndef GET_UINT32_LE
#define GET_UINT32_LE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ]       )             \
        | ( (uint32_t) (b)[(i) + 1] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 2] << 16 )             \
        | ( (uint32_t) (b)[(i) + 3] << 24 );            \
}
#endif

#ifndef PUT_UINT32_LE
#define PUT_UINT32_LE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n)       );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 3] = (unsigned char) ( (n) >> 24 );       \
}
#endif

/* Tables Generation Code */
#define ROTL8(x) ( ( x << 8 ) & 0xFFFFFFFF ) | ( x >> 24 )
#define XTIME(x) ( ( x << 1 ) ^ ( ( x & 0x80 ) ? 0x1B : 0x00 ) )
#define MUL(x,y) ( ( x && y ) ? pow[(log[x]+log[y]) % 255] : 0 )

#define AES_ENC_ROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)  \
{                                               \
    X0 = *RK++ ^ FTable0[ Y0 & 0xFF ] ^ FTable1[ ( Y1 >>  8 ) & 0xFF ] ^ FTable2[ ( Y2 >> 16 ) & 0xFF ] ^ FTable3[ ( Y3 >> 24 ) & 0xFF ]; \
    X1 = *RK++ ^ FTable0[ Y1 & 0xFF ] ^ FTable1[ ( Y2 >>  8 ) & 0xFF ] ^ FTable2[ ( Y3 >> 16 ) & 0xFF ] ^ FTable3[ ( Y0 >> 24 ) & 0xFF ]; \
    X2 = *RK++ ^ FTable0[ Y2 & 0xFF ] ^ FTable1[ ( Y3 >>  8 ) & 0xFF ] ^ FTable2[ ( Y0 >> 16 ) & 0xFF ] ^ FTable3[ ( Y1 >> 24 ) & 0xFF ]; \
    X3 = *RK++ ^ FTable0[ Y3 & 0xFF ] ^ FTable1[ ( Y0 >>  8 ) & 0xFF ] ^ FTable2[ ( Y1 >> 16 ) & 0xFF ] ^ FTable3[ ( Y2 >> 24 ) & 0xFF ]; \
}

#define AES_DEC_ROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)  \
{                                               \
    X0 = *RK++ ^ RTable0[ Y0 & 0xFF ] ^ RTable1[ ( Y3 >>  8 ) & 0xFF ] ^ RTable2[ ( Y2 >> 16 ) & 0xFF ] ^ RTable3[ ( Y1 >> 24 ) & 0xFF ]; \
    X1 = *RK++ ^ RTable0[ Y1 & 0xFF ] ^ RTable1[ ( Y0 >>  8 ) & 0xFF ] ^ RTable2[ ( Y3 >> 16 ) & 0xFF ] ^ RTable3[ ( Y2 >> 24 ) & 0xFF ]; \
    X2 = *RK++ ^ RTable0[ Y2 & 0xFF ] ^ RTable1[ ( Y1 >>  8 ) & 0xFF ] ^ RTable2[ ( Y0 >> 16 ) & 0xFF ] ^ RTable3[ ( Y3 >> 24 ) & 0xFF ]; \
    X3 = *RK++ ^ RTable0[ Y3 & 0xFF ] ^ RTable1[ ( Y2 >>  8 ) & 0xFF ] ^ RTable2[ ( Y1 >> 16 ) & 0xFF ] ^ RTable3[ ( Y0 >> 24 ) & 0xFF ]; \
}

int DAMO_CRYPT_AES_Set_EKey( AES_CTX *ctx, const unsigned char *key, unsigned int key_len )
{
    unsigned int i;
    uint32_t *RK;

    switch( key_len )
    {
        case 16: ctx->nr = 10; break;
        case 24: ctx->nr = 12; break;
        case 32: ctx->nr = 14; break;
        default : return DAMO_CRYPT_ERR_AES_ENC_INVALID_LEN;
    }

    ctx->rk = RK = ctx->buf;

    for( i = 0; i < ( key_len >> 2 ); i++ )
    {
        GET_UINT32_LE( RK[i], key, i << 2 );
    }

    switch( ctx->nr )
    {
        case 10:

            for( i = 0; i < 10; i++, RK += 4 )
            {
				RK[4]  = RK[0] ^ RCon[i] ^
					( (uint32_t) FSBox[ ( RK[3] >>  8 ) & 0xFF ] ) ^
					( (uint32_t) FSBox[ ( RK[3] >> 16 ) & 0xFF ] <<  8 ) ^
					( (uint32_t) FSBox[ ( RK[3] >> 24 ) & 0xFF ] << 16 ) ^
					( (uint32_t) FSBox[ RK[3] & 0xFF ] << 24 );

                RK[5]  = RK[1] ^ RK[4];
                RK[6]  = RK[2] ^ RK[5];
                RK[7]  = RK[3] ^ RK[6];
            }
            break;

        case 12:

            for( i = 0; i < 8; i++, RK += 6 )
            {
				RK[6]  = RK[0] ^ RCon[i] ^
					( (uint32_t) FSBox[ ( RK[5] >>  8 ) & 0xFF ] ) ^
					( (uint32_t) FSBox[ ( RK[5] >> 16 ) & 0xFF ] <<  8 ) ^
					( (uint32_t) FSBox[ ( RK[5] >> 24 ) & 0xFF ] << 16 ) ^
					( (uint32_t) FSBox[ RK[5] & 0xFF ] << 24 );

                RK[7]  = RK[1] ^ RK[6];
                RK[8]  = RK[2] ^ RK[7];
                RK[9]  = RK[3] ^ RK[8];
                RK[10] = RK[4] ^ RK[9];
                RK[11] = RK[5] ^ RK[10];
            }
            break;

        case 14:

            for( i = 0; i < 7; i++, RK += 8 )
            {
				RK[8]  = RK[0] ^ RCon[i] ^
					( (uint32_t) FSBox[ ( RK[7] >>  8 ) & 0xFF ] ) ^
					( (uint32_t) FSBox[ ( RK[7] >> 16 ) & 0xFF ] <<  8 ) ^
					( (uint32_t) FSBox[ ( RK[7] >> 24 ) & 0xFF ] << 16 ) ^
					( (uint32_t) FSBox[ RK[7] & 0xFF ] << 24 );

                RK[9]  = RK[1] ^ RK[8];
                RK[10] = RK[2] ^ RK[9];
                RK[11] = RK[3] ^ RK[10];

				RK[12] = RK[4] ^
					( (uint32_t) FSBox[ RK[11] & 0xFF ] ) ^
					( (uint32_t) FSBox[ ( RK[11] >>  8 ) & 0xFF ] <<  8 ) ^
					( (uint32_t) FSBox[ ( RK[11] >> 16 ) & 0xFF ] << 16 ) ^
					( (uint32_t) FSBox[ ( RK[11] >> 24 ) & 0xFF ] << 24 );

                RK[13] = RK[5] ^ RK[12];
                RK[14] = RK[6] ^ RK[13];
                RK[15] = RK[7] ^ RK[14];
            }
            break;
    }

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_AES_Set_DKey( AES_CTX *ctx, const unsigned char *key, unsigned int key_len )
{
    int i, j, ret;
    AES_CTX tmp_ctx;
    uint32_t *RK;
    uint32_t *SK;

    memset( &tmp_ctx, 0, sizeof( AES_CTX ) );

    ctx->rk = RK = ctx->buf;

    if( ( ret = DAMO_CRYPT_AES_Set_EKey( &tmp_ctx, key, key_len ) ) != 0 )
        goto exit;

    ctx->nr = tmp_ctx.nr;

    SK = tmp_ctx.rk + tmp_ctx.nr * 4;

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;

    for( i = ctx->nr - 1, SK -= 8; i > 0; i--, SK -= 8 )
    {
        for( j = 0; j < 4; j++, SK++ )
        {
			*RK++ = RTable0[ FSBox[ ( *SK ) & 0xFF ] ] ^
				RTable1[ FSBox[ ( *SK >>  8 ) & 0xFF ] ] ^
				RTable2[ FSBox[ ( *SK >> 16 ) & 0xFF ] ] ^
				RTable3[ FSBox[ ( *SK >> 24 ) & 0xFF ] ];
        }
    }

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;

exit:
	memset( &tmp_ctx, 0, sizeof( AES_CTX ) );

    return ret;
}

int DAMO_CRYPT_AES_Block( AES_CTX *ctx, int is_enc, const unsigned char input[16], unsigned char output[16] )
{
    int i;
    uint32_t *RK, X0, X1, X2, X3, Y0, Y1, Y2, Y3;

    RK = ctx->rk;

    GET_UINT32_LE( X0, input,  0 ); X0 ^= *RK++;
    GET_UINT32_LE( X1, input,  4 ); X1 ^= *RK++;
    GET_UINT32_LE( X2, input,  8 ); X2 ^= *RK++;
    GET_UINT32_LE( X3, input, 12 ); X3 ^= *RK++;

    if( is_enc == DAMO_CRYPT_DEC )
    {
        for( i = ( ctx->nr >> 1 ) - 1; i > 0; i-- )
        {
            AES_DEC_ROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
            AES_DEC_ROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
        }

        AES_DEC_ROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );

        X0 = *RK++ ^ ( (uint32_t) RSBox[ Y0 & 0xFF ] ) ^
			( (uint32_t) RSBox[ ( Y3 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) RSBox[ ( Y2 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) RSBox[ ( Y1 >> 24 ) & 0xFF ] << 24 );

        X1 = *RK++ ^ ( (uint32_t) RSBox[ Y1 & 0xFF ] ) ^
			( (uint32_t) RSBox[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) RSBox[ ( Y3 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) RSBox[ ( Y2 >> 24 ) & 0xFF ] << 24 );

        X2 = *RK++ ^ ( (uint32_t) RSBox[ Y2 & 0xFF ] ) ^
			( (uint32_t) RSBox[ ( Y1 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) RSBox[ ( Y0 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) RSBox[ ( Y3 >> 24 ) & 0xFF ] << 24 );

        X3 = *RK++ ^ ( (uint32_t) RSBox[ Y3 & 0xFF ] ) ^
			( (uint32_t) RSBox[ ( Y2 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) RSBox[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) RSBox[ ( Y0 >> 24 ) & 0xFF ] << 24 );
    }
    else
    {
        for( i = ( ctx->nr >> 1 ) - 1; i > 0; i-- )
        {
            AES_ENC_ROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
            AES_ENC_ROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
        }

        AES_ENC_ROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );

		X0 = *RK++ ^ ( (uint32_t) FSBox[ Y0 & 0xFF ] ) ^
			( (uint32_t) FSBox[ ( Y1 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSBox[ ( Y2 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSBox[ ( Y3 >> 24 ) & 0xFF ] << 24 );

		X1 = *RK++ ^ ( (uint32_t) FSBox[ Y1 & 0xFF ] ) ^
			( (uint32_t) FSBox[ ( Y2 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSBox[ ( Y3 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSBox[ ( Y0 >> 24 ) & 0xFF ] << 24 );

		X2 = *RK++ ^ ( (uint32_t) FSBox[ Y2 & 0xFF ] ) ^
			( (uint32_t) FSBox[ ( Y3 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSBox[ ( Y0 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSBox[ ( Y1 >> 24 ) & 0xFF ] << 24 );

		X3 = *RK++ ^ ( (uint32_t) FSBox[ Y3 & 0xFF ] ) ^
			( (uint32_t) FSBox[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSBox[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSBox[ ( Y2 >> 24 ) & 0xFF ] << 24 );
    }

    PUT_UINT32_LE( X0, output,  0 );
    PUT_UINT32_LE( X1, output,  4 );
    PUT_UINT32_LE( X2, output,  8 );
    PUT_UINT32_LE( X3, output, 12 );

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_AES_CBC( AES_CTX *ctx, int is_enc, unsigned char iv[16],
		const unsigned char *input, size_t input_len, unsigned char *output )
{
    int i;
    unsigned char temp[16];

    if( input_len % 16 )
        return DAMO_CRYPT_ERR_AES_ENC_INVALID_LEN ;


    if( is_enc == DAMO_CRYPT_DEC )
    {
        while( input_len > 0 )
        {
            memcpy( temp, input, 16 );
			DAMO_CRYPT_AES_Block( ctx, is_enc, input, output );

            for( i = 0; i < 16; i++ )
                output[i] = (unsigned char)( output[i] ^ iv[i] );

            memcpy( iv, temp, 16 );

            input  += 16;
            output += 16;
            input_len -= 16;
        }
    }
    else
    {
        while( input_len > 0 )
        {
            for( i = 0; i < 16; i++ )
                output[i] = (unsigned char)( input[i] ^ iv[i] );

            DAMO_CRYPT_AES_Block( ctx, is_enc, output, output );
            memcpy( iv, output, 16 );

            input  += 16;
            output += 16;
            input_len -= 16;
        }
    }

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_AES_CFB( AES_CTX *ctx, int is_enc, unsigned char iv[16], size_t *iv_off, 
		const unsigned char *input, size_t input_len, unsigned char *output )
{
    int c;
    size_t n = *iv_off;

    if( is_enc == DAMO_CRYPT_DEC )
    {
        while( input_len-- )
        {
            if( n == 0 )
                DAMO_CRYPT_AES_Block( ctx, DAMO_CRYPT_ENC, iv, iv );

            c = *input++;
            *output++ = (unsigned char)( c ^ iv[n] );
            iv[n] = (unsigned char) c;

            n = ( n + 1 ) & 0x0F;
        }
    }
    else
    {
        while( input_len-- )
        {
            if( n == 0 )
                DAMO_CRYPT_AES_Block( ctx, DAMO_CRYPT_ENC, iv, iv );

            iv[n] = *output++ = (unsigned char)( iv[n] ^ *input++ );

            n = ( n + 1 ) & 0x0F;
        }
    }

    *iv_off = n;

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_AES_Encrypt_Core(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, int pad)
{
	int ret;
	AES_CTX ctx;

	if(!out || !in || !key)
		return DAMO_CRYPT_ERR_AES_ENC_NULL_POINTER;

	if(alg == AES_128)
	{
		if(key_len < 16)
			return DAMO_CRYPT_ERR_AES_ENC_INVALID_LEN;
	}
	else if(alg == AES_192)
	{
		if(key_len < 24)
			return DAMO_CRYPT_ERR_AES_ENC_INVALID_LEN;
	}
	else if(alg == AES_256)
	{
		if(key_len < 32)
			return DAMO_CRYPT_ERR_AES_ENC_INVALID_LEN;
	}
	else
	{
       return DAMO_CRYPT_ERR_AES_ENC_INVALID_ALG;
	}

	ret = DAMO_CRYPT_AES_Set_EKey( &ctx, key, (unsigned int)key_len );
	if(ret < 0)
		return DAMO_CRYPT_ERR_AES_ENC_KEY_SCH_FAIL;

	if(mode == CBC_MODE)
	{
		unsigned char rm = 0;

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
		unsigned char *tmp_in = NULL;
#else
    unsigned char tmp_in[INLENMAX];
#endif

  if ( pad == DAMO_PKCS7_PADDING )
    rm = (unsigned char)(16 - (in_len % 16));
  else if ( pad == DAMO_ZERO_PADDING )
  {
    if (in_len % 16 == 0)
    {
      rm = 0;
    }
    else
    {
      rm = (unsigned char)(16 - (in_len % 16));
    }
  }
  else
  {
	  return DAMO_CRYPT_ERR_AES_ENC_INVALID_PAD;
  }
    

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
		tmp_in = (unsigned char *) malloc(in_len + rm);
#else
    if(INLENMAX < in_len + rm)
      return DAMO_CRYPT_ERR_AES_ENC_INVALID_LEN;
#endif
	
	memcpy(tmp_in, in, in_len);


	if( pad == DAMO_PKCS7_PADDING )
    {
	  //servier pkcs7 padding
	  memset(tmp_in + in_len, rm, rm);
	}
	else
	{
	  //server zero padding
	  memset(tmp_in + in_len, 0x00, rm);
	}


	ret = DAMO_CRYPT_AES_CBC( &ctx, DAMO_CRYPT_ENC, ivec, tmp_in, in_len+rm, out );
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
  free(tmp_in);
#endif
    if ( ret != 0)
      return ret;

		*out_len = in_len + rm;
	}
	else if(mode == CFB_MODE)
	{
		size_t iv_off = 0;

		ret = DAMO_CRYPT_AES_CFB( &ctx, DAMO_CRYPT_ENC, ivec, &iv_off, in, in_len, out );

		*out_len = in_len;
	}
	else
		return DAMO_CRYPT_ERR_AES_ENC_INVALID_MODE;

	return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_AES_Decrypt_Core(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec, int pad)
{
	int ret;
	AES_CTX ctx;
	size_t i = 0;
	if(!out || !in || !key)
		return DAMO_CRYPT_ERR_AES_DEC_NULL_POINTER;

	if(alg == AES_128)
	{
		if(key_len < 16)
			return DAMO_CRYPT_ERR_AES_DEC_INVALID_LEN;
	}
	else if(alg == AES_192)
	{
		if(key_len < 24)
			return DAMO_CRYPT_ERR_AES_DEC_INVALID_LEN;
	}
	else if(alg == AES_256)
	{
		if(key_len < 32)
			return DAMO_CRYPT_ERR_AES_DEC_INVALID_LEN;
	}
	else
	{
		return DAMO_CRYPT_ERR_AES_DEC_INVALID_ALG;
	}

  if(mode == CBC_MODE)
  {
    unsigned char rm = 0;

    ret = DAMO_CRYPT_AES_Set_DKey( &ctx, key, (unsigned int)key_len );
    if(ret < 0)
      return DAMO_CRYPT_ERR_AES_DEC_KEY_SCH_FAIL;

    ret = DAMO_CRYPT_AES_CBC( &ctx, DAMO_CRYPT_DEC, ivec, in, in_len, out );
    if(ret != 0 )
      return ret;

    rm = out[in_len - 1];


    if( pad == DAMO_ZERO_PADDING )
    {
      //servier zero padding
      for(i = in_len-1 ; i > 0 ; i--)
      {
        if(out[i] != '\0')
        break;
      }
      rm = 0;
      *out_len = i + 1;
      
    }
    else if( pad == DAMO_PKCS7_PADDING )
    {
      //server pkcs7 padding
      if ( rm == 0 || rm > 16 )
      {
        *out_len = 0;
        return DAMO_CRYPT_ERR_AES_DEC_INVALID_PAD;
      }

      *out_len = in_len - rm;
    }
    else
    {
      return DAMO_CRYPT_ERR_AES_DEC_INVALID_PAD;
    }

    memset(out + (*out_len), 0, rm);
  }
	else if(mode == CFB_MODE)
	{
		size_t iv_off = 0;

		ret = DAMO_CRYPT_AES_Set_EKey( &ctx, key, (unsigned int)key_len );
		if(ret < 0)
			return DAMO_CRYPT_ERR_AES_DEC_KEY_SCH_FAIL;

		ret = DAMO_CRYPT_AES_CFB( &ctx, DAMO_CRYPT_DEC, ivec, &iv_off, in, in_len, out );

		*out_len = in_len;
	}
	else
		return DAMO_CRYPT_ERR_AES_DEC_INVALID_MODE;

	return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_AES_Encrypt(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode)
{
  int ret;
  unsigned char in_ivec[16]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

#if defined(DUKPT_SERVER)
  ret = DAMO_CRYPT_AES_Encrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, initValues.padding);
#elif defined(DUKPT_CLIENT_ZERO_PADDING)
  ret = DAMO_CRYPT_AES_Encrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_ZERO_PADDING);
#else
  ret = DAMO_CRYPT_AES_Encrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_PKCS7_PADDING);
#endif

  return ret;
}

int DAMO_CRYPT_AES_Decrypt(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode)
{
  int ret;
  unsigned char in_ivec[16]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

#if defined(DUKPT_SERVER)
  ret = DAMO_CRYPT_AES_Decrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, initValues.padding);
#elif defined(DUKPT_CLIENT_ZERO_PADDING)
  ret = DAMO_CRYPT_AES_Decrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_ZERO_PADDING);
#else
  ret = DAMO_CRYPT_AES_Decrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_PKCS7_PADDING);
#endif

  return ret;
}

int DAMO_CRYPT_AES_EncryptEx(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec)
{
  int ret;
  unsigned char in_ivec[16] = {0};
  if (ivec != NULL)
    memcpy(in_ivec, ivec, 16);

#if defined(DUKPT_SERVER)
  ret = DAMO_CRYPT_AES_Encrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, initValues.padding);
#elif defined(DUKPT_CLIENT_ZERO_PADDING)
  ret = DAMO_CRYPT_AES_Encrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_ZERO_PADDING);
#else
  ret = DAMO_CRYPT_AES_Encrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_PKCS7_PADDING);
#endif
  
  return ret;
}

int DAMO_CRYPT_AES_DecryptEx(unsigned char *out, size_t *out_len,
		const unsigned char *in, size_t in_len, const unsigned char *key, size_t key_len, int alg, int mode, unsigned char *ivec)
{
  int ret;
  unsigned char in_ivec[16] = {0};
  if (ivec != NULL)
    memcpy(in_ivec, ivec, 16);
#if defined(DUKPT_SERVER)
  ret = DAMO_CRYPT_AES_Decrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, initValues.padding);
#elif defined(DUKPT_CLIENT_ZERO_PADDING)
  ret = DAMO_CRYPT_AES_Decrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_ZERO_PADDING);
#else
  ret = DAMO_CRYPT_AES_Decrypt_Core(out, out_len, in, in_len, key, key_len, alg, mode, in_ivec, DAMO_PKCS7_PADDING);
#endif

  return ret;
}