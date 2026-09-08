/*
 *  Public Key abstraction layer: wrapper functions
 *
 *  Copyright (C) 2006-2014, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://polarssl.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "ps_config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_PK_C)

#include "ps_pk_wrap.h"

/* Even if RSA not activated, for the sake of RSA-alt */
#include "ps_rsa.h"

#include <stdlib.h>
#define polarssl_malloc     malloc
#define polarssl_free       free

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#if defined(POLARSSL_RSA_C)
static int rsa_can_do( pk_type_t type )
{
    return( type == POLARSSL_PK_RSA ||
            type == POLARSSL_PK_RSASSA_PSS );
}

static size_t rsa_get_size( const void *ctx )
{
    return( 8 * ((const rsa_context *) ctx)->len );
}

static int rsa_verify_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   const unsigned char *sig, size_t sig_len )
{
    int ret;

    if( sig_len < ((rsa_context *) ctx)->len )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( ( ret = rsa_pkcs1_verify( (rsa_context *) ctx,
                                  RSA_PUBLIC, md_alg,
                                  (unsigned int) hash_len, hash, sig ) ) != 0 )
        return( ret );

    if( sig_len > ((rsa_context *) ctx)->len )
        return( POLARSSL_ERR_PK_SIG_LEN_MISMATCH );

    return( 0 );
}

static int rsa_sign_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   unsigned char *sig, size_t *sig_len)
{
    *sig_len = ((rsa_context *) ctx)->len;

    return( rsa_pkcs1_sign( (rsa_context *) ctx, RSA_PRIVATE,
                md_alg, (unsigned int) hash_len, hash, sig ) );
}

static int rsa_decrypt_wrap( void *ctx,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output, size_t *olen, size_t osize)
{
    if( ilen != ((rsa_context *) ctx)->len )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    return( rsa_pkcs1_decrypt( (rsa_context *) ctx,
                RSA_PRIVATE, olen, input, output, osize ) );
}

static int rsa_encrypt_wrap( void *ctx,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output, size_t *olen, size_t osize)
{
    *olen = ((rsa_context *) ctx)->len;

    if( *olen > osize )
        return( POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE );

    return( rsa_pkcs1_encrypt( (rsa_context *) ctx,
                RSA_PUBLIC, ilen, input, output ) );
}

static int rsa_check_pair_wrap( const void *pub, const void *prv )
{
    return( rsa_check_pub_priv( (const rsa_context *) pub,
                                (const rsa_context *) prv ) );
}

static void *rsa_alloc_wrap( void )
{
    void *ctx = polarssl_malloc( sizeof( rsa_context ) );

    if( ctx != NULL )
        rsa_init( (rsa_context *) ctx, 0, 0 );

    return( ctx );
}

static void rsa_free_wrap( void *ctx )
{
    rsa_free( (rsa_context *) ctx );
    polarssl_free( ctx );
}

static void rsa_debug( const void *ctx, pk_debug_item *items )
{
    items->type = POLARSSL_PK_DEBUG_MPI;
    items->name = "rsa.N";
    items->value = &( ((rsa_context *) ctx)->N );

    items++;

    items->type = POLARSSL_PK_DEBUG_MPI;
    items->name = "rsa.E";
    items->value = &( ((rsa_context *) ctx)->E );
}

const pk_info_t rsa_info = {
    POLARSSL_PK_RSA,
    "RSA",
    rsa_get_size,
    rsa_can_do,
    rsa_verify_wrap,
    rsa_sign_wrap,
    rsa_decrypt_wrap,
    rsa_encrypt_wrap,
    rsa_check_pair_wrap,
    rsa_alloc_wrap,
    rsa_free_wrap,
    rsa_debug,
};
#endif /* POLARSSL_RSA_C */

/*
 * Support for alternative RSA-private implementations
 */

static int rsa_alt_can_do( pk_type_t type )
{
    return( type == POLARSSL_PK_RSA );
}

static size_t rsa_alt_get_size( const void *ctx )
{
    const rsa_alt_context *rsa_alt = (const rsa_alt_context *) ctx;

    return( 8 * rsa_alt->key_len_func( rsa_alt->key ) );
}

static int rsa_alt_sign_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   unsigned char *sig, size_t *sig_len)
{
    rsa_alt_context *rsa_alt = (rsa_alt_context *) ctx;

    *sig_len = rsa_alt->key_len_func( rsa_alt->key );

    return( rsa_alt->sign_func( rsa_alt->key, RSA_PRIVATE,
                md_alg, (unsigned int) hash_len, hash, sig ) );
}

static int rsa_alt_decrypt_wrap( void *ctx,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output, size_t *olen, size_t osize)
{
    rsa_alt_context *rsa_alt = (rsa_alt_context *) ctx;


    if( ilen != rsa_alt->key_len_func( rsa_alt->key ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    return( rsa_alt->decrypt_func( rsa_alt->key,
                RSA_PRIVATE, olen, input, output, osize ) );
}

#if defined(POLARSSL_RSA_C)
static int rsa_alt_check_pair( const void *pub, const void *prv )
{
    unsigned char sig[POLARSSL_MPI_MAX_SIZE];
    unsigned char hash[32];
    size_t sig_len = 0;
    int ret;

    if( rsa_alt_get_size( prv ) != rsa_get_size( pub ) )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    memset( hash, 0x2a, sizeof( hash ) );

    if( ( ret = rsa_alt_sign_wrap( (void *) prv, POLARSSL_MD_NONE,
                                   hash, sizeof( hash ),
                                   sig, &sig_len) ) != 0 )
    {
        return( ret );
    }

    if( rsa_verify_wrap( (void *) pub, POLARSSL_MD_NONE,
                         hash, sizeof( hash ), sig, sig_len ) != 0 )
    {
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );
    }

    return( 0 );
}
#endif /* POLARSSL_RSA_C */

static void *rsa_alt_alloc_wrap( void )
{
    void *ctx = polarssl_malloc( sizeof( rsa_alt_context ) );

    if( ctx != NULL )
        memset( ctx, 0, sizeof( rsa_alt_context ) );

    return( ctx );
}

static void rsa_alt_free_wrap( void *ctx )
{
    polarssl_zeroize( ctx, sizeof( rsa_alt_context ) );
    polarssl_free( ctx );
}

const pk_info_t rsa_alt_info = {
    POLARSSL_PK_RSA_ALT,
    "RSA-alt",
    rsa_alt_get_size,
    rsa_alt_can_do,
    NULL,
    rsa_alt_sign_wrap,
    rsa_alt_decrypt_wrap,
    NULL,
#if defined(POLARSSL_RSA_C)
    rsa_alt_check_pair,
#else
    NULL,
#endif
    rsa_alt_alloc_wrap,
    rsa_alt_free_wrap,
    NULL,
};

#endif /* POLARSSL_PK_C */
