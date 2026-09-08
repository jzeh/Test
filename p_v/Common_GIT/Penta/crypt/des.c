#include "des.h"
#include "des_tables.h"
#ifdef DUKPT_SERVER
	#include "dukpt_server_module.h"
#else
    #include "dukpt.h"
#endif

/* 32-bit Integer Manipulation Macros (Big Endian) */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

/* Initial Permutation Macro */
#define DES_IP(X,Y)                                             \
{                                                               \
    T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
    T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
    T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
    T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
    Y = ((Y << 1) | (Y >> 31)) & 0xFFFFFFFF;                    \
    T = (X ^ Y) & 0xAAAAAAAA; Y ^= T; X ^= T;                   \
    X = ((X << 1) | (X >> 31)) & 0xFFFFFFFF;                    \
}

/* Final Permutation Macro */
#define DES_FP(X,Y)                                             \
{                                                               \
    X = ((X << 31) | (X >> 1)) & 0xFFFFFFFF;                    \
    T = (X ^ Y) & 0xAAAAAAAA; X ^= T; Y ^= T;                   \
    Y = ((Y << 31) | (Y >> 1)) & 0xFFFFFFFF;                    \
    T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
    T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
    T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
    T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
}

/* DES Round Macro */
#define DES_ROUND(X,Y)                          					\
{                                               					\
    T = *SK++ ^ X;                              					\
	Y ^= SBox1[ (T >> 24) & 0x3F ] ^ SBox3[ (T >> 16) & 0x3F ] ^	\
		 SBox5[ (T >>  8) & 0x3F ] ^ SBox7[ (T      ) & 0x3F ];		\
    T = *SK++ ^ ((X << 28) | (X >> 4));         					\
	Y ^= SBox0[ (T >> 24) & 0x3F ] ^ SBox2[ (T >> 16) & 0x3F ] ^	\
		 SBox4[ (T >>  8) & 0x3F ] ^ SBox6[ (T      ) & 0x3F ];		\
}

#define SWAP(a,b) { uint32_t t = a; a = b; b = t; t = 0; }

#define WEAK_KEY_COUNT 16

void DAMO_CRYPT_DES_Key_Set_Parity( unsigned char key[DES_KEY_SIZE] )
{
    int i;

    for( i = 0; i < DES_KEY_SIZE; i++ )
        key[i] = odd_parity[key[i] / 2];
}

int DAMO_CRYPT_DES_Key_Check_Parity( const unsigned char key[DES_KEY_SIZE] )
{
    int i;

    for( i = 0; i < DES_KEY_SIZE; i++ )
        if( key[i] != odd_parity[key[i] / 2] )
            return -1;

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES_Key_Check_Weak( const unsigned char key[DES_KEY_SIZE] )
{
    int i;

    for( i = 0; i < WEAK_KEY_COUNT; i++ )
        if( memcmp( weak_key_table[i], key, DES_KEY_SIZE) == 0 )
            return -1;

    return DAMO_CRYPT_SUCCESS;
}

static void DAMO_CRYPT_DES_Set_Key( uint32_t SK[32], const unsigned char key[DES_KEY_SIZE] )
{
    int i;
    uint32_t X, Y, T;

    GET_UINT32_BE( X, key, 0 );
    GET_UINT32_BE( Y, key, 4 );

    T =  ((Y >>  4) ^ X) & 0x0F0F0F0F;  X ^= T; Y ^= (T <<  4);
    T =  ((Y      ) ^ X) & 0x10101010;  X ^= T; Y ^= (T      );

    X =   (LHBSwap[ (X      ) & 0xF] << 3) | (LHBSwap[ (X >>  8) & 0xF ] << 2)
        | (LHBSwap[ (X >> 16) & 0xF] << 1) | (LHBSwap[ (X >> 24) & 0xF ]     )
        | (LHBSwap[ (X >>  5) & 0xF] << 7) | (LHBSwap[ (X >> 13) & 0xF ] << 6)
        | (LHBSwap[ (X >> 21) & 0xF] << 5) | (LHBSwap[ (X >> 29) & 0xF ] << 4);

    Y =   (RHBSwap[ (Y >>  1) & 0xF] << 3) | (RHBSwap[ (Y >>  9) & 0xF ] << 2)
        | (RHBSwap[ (Y >> 17) & 0xF] << 1) | (RHBSwap[ (Y >> 25) & 0xF ]     )
        | (RHBSwap[ (Y >>  4) & 0xF] << 7) | (RHBSwap[ (Y >> 12) & 0xF ] << 6)
        | (RHBSwap[ (Y >> 20) & 0xF] << 5) | (RHBSwap[ (Y >> 28) & 0xF ] << 4);

    X &= 0x0FFFFFFF;
    Y &= 0x0FFFFFFF;

    for( i = 0; i < 16; i++ )
    {
        if( i < 2 || i == 8 || i == 15 )
        {
            X = ((X <<  1) | (X >> 27)) & 0x0FFFFFFF;
            Y = ((Y <<  1) | (Y >> 27)) & 0x0FFFFFFF;
        }
        else
        {
            X = ((X <<  2) | (X >> 26)) & 0x0FFFFFFF;
            Y = ((Y <<  2) | (Y >> 26)) & 0x0FFFFFFF;
        }

        *SK++ =   ((X <<  4) & 0x24000000) | ((X << 28) & 0x10000000)
                | ((X << 14) & 0x08000000) | ((X << 18) & 0x02080000)
                | ((X <<  6) & 0x01000000) | ((X <<  9) & 0x00200000)
                | ((X >>  1) & 0x00100000) | ((X << 10) & 0x00040000)
                | ((X <<  2) & 0x00020000) | ((X >> 10) & 0x00010000)
                | ((Y >> 13) & 0x00002000) | ((Y >>  4) & 0x00001000)
                | ((Y <<  6) & 0x00000800) | ((Y >>  1) & 0x00000400)
                | ((Y >> 14) & 0x00000200) | ((Y      ) & 0x00000100)
                | ((Y >>  5) & 0x00000020) | ((Y >> 10) & 0x00000010)
                | ((Y >>  3) & 0x00000008) | ((Y >> 18) & 0x00000004)
                | ((Y >> 26) & 0x00000002) | ((Y >> 24) & 0x00000001);

        *SK++ =   ((X << 15) & 0x20000000) | ((X << 17) & 0x10000000)
                | ((X << 10) & 0x08000000) | ((X << 22) & 0x04000000)
                | ((X >>  2) & 0x02000000) | ((X <<  1) & 0x01000000)
                | ((X << 16) & 0x00200000) | ((X << 11) & 0x00100000)
                | ((X <<  3) & 0x00080000) | ((X >>  6) & 0x00040000)
                | ((X << 15) & 0x00020000) | ((X >>  4) & 0x00010000)
                | ((Y >>  2) & 0x00002000) | ((Y <<  8) & 0x00001000)
                | ((Y >> 14) & 0x00000808) | ((Y >>  9) & 0x00000400)
                | ((Y      ) & 0x00000200) | ((Y <<  7) & 0x00000100)
                | ((Y >>  7) & 0x00000020) | ((Y >>  3) & 0x00000011)
                | ((Y <<  2) & 0x00000004) | ((Y >> 21) & 0x00000002);
    }
}

int DAMO_CRYPT_DES_Set_EKey( DES_CTX *ctx, const unsigned char key[DES_KEY_SIZE] )
{
    DAMO_CRYPT_DES_Set_Key( ctx->sk, key );

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES_Set_DKey( DES_CTX *ctx, const unsigned char key[DES_KEY_SIZE] )
{
    int i;

    DAMO_CRYPT_DES_Set_Key( ctx->sk, key );

    for( i = 0; i < 16; i += 2 )
    {
        SWAP( ctx->sk[i    ], ctx->sk[30 - i] );
        SWAP( ctx->sk[i + 1], ctx->sk[31 - i] );
    }

    return DAMO_CRYPT_SUCCESS;
}

static void DAMO_CRYPT_DES3_Set_Key2( uint32_t esk[96], uint32_t dsk[96], const unsigned char key[DES_KEY_SIZE*2] )
{
    int i;

    DAMO_CRYPT_DES_Set_Key( esk, key );
    DAMO_CRYPT_DES_Set_Key( dsk + 32, key + 8 );

    for( i = 0; i < 32; i += 2 )
    {
        dsk[i     ] = esk[30 - i];
        dsk[i +  1] = esk[31 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        esk[i + 64] = esk[i    ];
        esk[i + 65] = esk[i + 1];

        dsk[i + 64] = dsk[i    ];
        dsk[i + 65] = dsk[i + 1];
    }
}

int DAMO_CRYPT_DES3_Set_EKey2( DES3_CTX *ctx, const unsigned char key[DES_KEY_SIZE * 2] )
{
    uint32_t sk[96];

    DAMO_CRYPT_DES3_Set_Key2( ctx->sk, sk, key );
    memset( sk, 0x00, sizeof( sk ) );

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES3_Set_DKey2( DES3_CTX *ctx, const unsigned char key[DES_KEY_SIZE * 2] )
{
    uint32_t sk[96];

    DAMO_CRYPT_DES3_Set_Key2( sk, ctx->sk, key );
    memset( sk, 0x00, sizeof( sk ) );

    return DAMO_CRYPT_SUCCESS;
}

static void DAMO_CRYPT_DES3_Set_Key3( uint32_t esk[96], uint32_t dsk[96], const unsigned char key[24] )
{
    int i;

    DAMO_CRYPT_DES_Set_Key( esk, key );
    DAMO_CRYPT_DES_Set_Key( dsk + 32, key +  8 );
    DAMO_CRYPT_DES_Set_Key( esk + 64, key + 16 );

    for( i = 0; i < 32; i += 2 )
    {
        dsk[i     ] = esk[94 - i];
        dsk[i +  1] = esk[95 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        dsk[i + 64] = esk[30 - i];
        dsk[i + 65] = esk[31 - i];
    }
}

int DAMO_CRYPT_DES3_Set_EKey3( DES3_CTX *ctx, const unsigned char key[DES_KEY_SIZE * 3] )
{
    uint32_t sk[96];

    DAMO_CRYPT_DES3_Set_Key3( ctx->sk, sk, key );
    memset( sk, 0x00, sizeof( sk ) );

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES3_Set_DKey3( DES3_CTX *ctx, const unsigned char key[DES_KEY_SIZE * 3] )
{
    uint32_t sk[96];

    DAMO_CRYPT_DES3_Set_Key3( sk, ctx->sk, key );
    memset( sk, 0x00, sizeof( sk ) );

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES_Block( DES_CTX *ctx, const unsigned char input[8], unsigned char output[8] )
{
    int i;
    uint32_t X, Y, T, *SK;

    SK = ctx->sk;

    GET_UINT32_BE( X, input, 0 );
    GET_UINT32_BE( Y, input, 4 );

    DES_IP( X, Y );

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    DES_FP( Y, X );

    PUT_UINT32_BE( Y, output, 0 );
    PUT_UINT32_BE( X, output, 4 );

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES_CBC( DES_CTX *ctx, int is_enc, unsigned char iv[8],
		const unsigned char *input, size_t input_len, unsigned char *output )
{
    int i;
    unsigned char temp[8];

    if( input_len % 8 )
        return DAMO_CRYPT_ERR_DES_ENC_INVALID_LEN;

    if( is_enc == DAMO_CRYPT_DES_ENC )
    {
        while( input_len > 0 )
        {
            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( input[i] ^ iv[i] );

            DAMO_CRYPT_DES_Block( ctx, output, output );
            memcpy( iv, output, 8 );

            input  += 8;
            output += 8;
            input_len -= 8;
        }
    }
    else
    {
        while( input_len > 0 )
        {
            memcpy( temp, input, 8 );
            DAMO_CRYPT_DES_Block( ctx, input, output );

            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( output[i] ^ iv[i] );

            memcpy( iv, temp, 8 );

            input  += 8;
            output += 8;
            input_len -= 8;
        }
    }

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES3_Block( DES3_CTX *ctx, const unsigned char input[8], unsigned char output[8] )
{
    int i;
    uint32_t X, Y, T, *SK;

    SK = ctx->sk;

    GET_UINT32_BE( X, input, 0 );
    GET_UINT32_BE( Y, input, 4 );

    DES_IP( X, Y );

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( X, Y );
        DES_ROUND( Y, X );
    }

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    DES_FP( Y, X );

    PUT_UINT32_BE( Y, output, 0 );
    PUT_UINT32_BE( X, output, 4 );

    return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_DES3_CBC( DES3_CTX *ctx, int is_enc, unsigned char iv[8],
		const unsigned char *input, size_t input_len, unsigned char *output )
{
    int i;
    unsigned char temp[8];
    unsigned char ivec[8] = {0};

    memcpy(ivec, iv, 8);

    if( input_len % 8 )
        return DAMO_CRYPT_ERR_TDES_ENC_INVALID_LEN;

    if( is_enc == DAMO_CRYPT_DES_ENC )
    {
        while( input_len > 0 )
        {
            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( input[i] ^ ivec[i] );

            DAMO_CRYPT_DES3_Block( ctx, output, output );
            memcpy( ivec, output, 8 );

            input  += 8;
            output += 8;
            input_len -= 8;
        }
    }
    else
    {
        while( input_len > 0 )
        {
            memcpy( temp, input, 8 );
            DAMO_CRYPT_DES3_Block( ctx, input, output );

            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( output[i] ^ ivec[i] );

            memcpy( ivec, temp, 8 );

            input  += 8;
            output += 8;
            input_len -= 8;
        }
    }

    return DAMO_CRYPT_SUCCESS;
}

int TDES_CBC_Encrypt(unsigned char *out, size_t *outLen, const unsigned char *in, int inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad)
{
    DES3_CTX ctx;
	unsigned char rm = 0;

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
	unsigned char *tmp_in = NULL;
#else
    unsigned char tmp_in[INLENMAX];
#endif

  if ( pad == DAMO_PKCS7_PADDING )
  {
    rm = (unsigned char)(8 - (inLen % 8));
  }
  else if( pad == DAMO_ZERO_PADDING )
  {
    if ( inLen % 8 == 0)
    {
      rm = 0;
    }
    else
    {
      rm = (unsigned char)(8 - (inLen % 8));
    }
  }
  else
  {
    return DAMO_CRYPT_ERR_TDES_ENC_INVALID_PAD;
  }


#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
	tmp_in = (unsigned char *) malloc(inLen + rm);
#else
    if(INLENMAX < inLen + rm)
      return DAMO_CRYPT_ERR_HIGHT_ENC_INVALID_LEN;
#endif

  memcpy(tmp_in, in, inLen);

  if( pad == DAMO_PKCS7_PADDING )
  {
	//servier pkcs7 padding
    memset(tmp_in + inLen, rm, rm);
  }
  else
  {
	//server zero padding
  	memset(tmp_in + inLen, 0x00, rm);
  }

  DAMO_CRYPT_DES3_Set_EKey2(&ctx, key);
  DAMO_CRYPT_DES3_CBC(&ctx, 1, iv, tmp_in, inLen+rm, out);

  *outLen = inLen+rm;

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
  free(tmp_in);
#endif

    return DAMO_CRYPT_SUCCESS;
}

int TDES_CBC_Decrypt(unsigned char *out, size_t *outLen, const unsigned char *in, int inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad)
{

  DES3_CTX ctx;
  unsigned char rm = 0;
  int i=0;

  DAMO_CRYPT_DES3_Set_DKey2(&ctx, key);
  DAMO_CRYPT_DES3_CBC(&ctx, 0, iv, in, inLen, out);

  rm = out[inLen - 1];

  if( pad == DAMO_ZERO_PADDING )
  {
	  //servier zero padding
	  for(i = inLen-1 ; i > 0 ; i--)
	  {
		  if(out[i] != '\0')
			  break;
	  }
    rm = 0;
	  *outLen = i + 1;

  }
  else if( pad == DAMO_PKCS7_PADDING )
  {
	  //server pkcs7 padding
	  if ( rm == 0 || rm > 8 )
	  {
		  *outLen = 0;
		  return DAMO_CRYPT_ERR_TDES_DEC_INVALID_PAD;
	  }

	  *outLen = inLen - rm;
  }
  else
  {
	  return DAMO_CRYPT_ERR_TDES_DEC_INVALID_PAD;
  }
  memset(out + (*outLen), 0, rm);

  return DAMO_CRYPT_SUCCESS;
}

int TDES_CFB_Encrypt(unsigned char *in, unsigned int inLen, const unsigned char *key, unsigned char *iv, unsigned int blockLen)
{
  DES3_CTX ctx;
  unsigned char out[8];
  DAMO_CRYPT_DES3_Set_DKey2(&ctx, key);

  memcpy(ctx.sk, iv, 8);
  
  while(1 <= inLen)
  {
	DAMO_CRYPT_DES3_Block(&ctx, key, out );
	*in ^= out[0];
	memmove(ctx.sk, ctx.sk+1, 8-1);
	ctx.sk[8-1] = *in;
	inLen--;
	in++;
  }
  return inLen;
}

int TDES_CFB_Decrypt(unsigned char *in, unsigned int inLen, const unsigned char *key, unsigned char *iv, unsigned int blockLen)
{
  DES3_CTX ctx;
  unsigned char out[8];
  unsigned char c;

  DAMO_CRYPT_DES3_Set_DKey2(&ctx, key);
  memcpy(ctx.sk, iv, 8);

  while (1 <= inLen) {
    c = *in;
    DAMO_CRYPT_DES3_Block(&ctx, key, out);
    *in ^= out[0];
    memmove(ctx.sk, ctx.sk+1, 8-1);
    ctx.sk[8-1] = c;
    inLen--;
    in++;
  }
  return inLen;
}

int DAMO_CRYPT_TDES_Encrypt_Core(unsigned char *out, size_t *outLen, const unsigned char *in, int inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad )
{

  int ret = -1;

  switch(mode)
  {
    case CBC_MODE:
    {
      ret = TDES_CBC_Encrypt(out, outLen, in, inLen, key, mode, iv, pad);
	}
	break;
	case CFB_MODE:
	{
	  memcpy(out, in, inLen);
      ret = TDES_CFB_Encrypt(out, inLen, key, iv, 8);
	  *outLen = inLen;
	}
    break;
  }	

  return ret;
}

int DAMO_CRYPT_TDES_Decrypt_Core(unsigned char *out, size_t *outLen, const unsigned char *in, int inLen, const unsigned char *key, int mode, unsigned char *iv, size_t pad)
{
  int ret = -1;

  switch(mode)
  {
    case CBC_MODE:
    {
      ret = TDES_CBC_Decrypt(out, outLen, in, inLen, key, mode, iv, pad);
	}
	break;
	case CFB_MODE:
	{
	  memcpy(out, in, inLen);
      ret = TDES_CFB_Decrypt(out, inLen, key, iv, 8);
	  *outLen = inLen;
	}
    break;
  }

  return ret;
}
