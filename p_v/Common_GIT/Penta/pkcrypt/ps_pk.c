/*
 *  Public Key abstraction layer
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

#include "ps_pk.h"
#include "ps_pk_wrap.h"

#if defined(POLARSSL_RSA_C)
#include "ps_rsa.h"
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 * Initialise a pk_context
 */
void pk_init( pk_context *ctx )
{
    if( ctx == NULL )
        return;

    ctx->pk_info = NULL;
    ctx->pk_ctx = NULL;
}

/*
 * Free (the components of) a pk_context
 */
void pk_free( pk_context *ctx )
{
    if( ctx == NULL || ctx->pk_info == NULL )
        return;

    ctx->pk_info->ctx_free_func( ctx->pk_ctx );

    polarssl_zeroize( ctx, sizeof( pk_context ) );
}

/*
 * Get pk_info structure from type
 */
const pk_info_t * pk_info_from_type( pk_type_t pk_type )
{
    switch( pk_type ) {
#if defined(POLARSSL_RSA_C)
        case POLARSSL_PK_RSA:
            return( &rsa_info );
#endif
        /* POLARSSL_PK_RSA_ALT omitted on purpose */
        default:
            return( NULL );
    }
}

/*
 * Initialise context
 */
int pk_init_ctx( pk_context *ctx, const pk_info_t *info )
{
    if( ctx == NULL || info == NULL || ctx->pk_info != NULL )
        return( POLARSSL_ERR_PK_BAD_INPUT_DATA );

    if( ( ctx->pk_ctx = info->ctx_alloc_func() ) == NULL )
        return( POLARSSL_ERR_PK_MALLOC_FAILED );

    ctx->pk_info = info;

    return( 0 );
}

/*
 * Helper for pk_sign and pk_verify
 */
static inline int pk_hashlen_helper( md_type_t md_alg, size_t *hash_len )
{
    const md_info_t *md_info;

    if( *hash_len != 0 )
        return( 0 );

    if( ( md_info = md_info_from_type( md_alg ) ) == NULL )
        return( -1 );

    *hash_len = md_info->size;
    return( 0 );
}

/*
 * Verify a signature
 */
int pk_verify( pk_context *ctx, md_type_t md_alg,
               const unsigned char *hash, size_t hash_len,
               const unsigned char *sig, size_t sig_len )
{
    if( ctx == NULL || ctx->pk_info == NULL ||
        pk_hashlen_helper( md_alg, &hash_len ) != 0 )
        return( POLARSSL_ERR_PK_BAD_INPUT_DATA );

    if( ctx->pk_info->verify_func == NULL )
        return( POLARSSL_ERR_PK_TYPE_MISMATCH );

    return( ctx->pk_info->verify_func( ctx->pk_ctx, md_alg, hash, hash_len,
                                       sig, sig_len ) );
}

/*
 * Make a signature
 */
int pk_sign( pk_context *ctx, md_type_t md_alg,
             const unsigned char *hash, size_t hash_len,
             unsigned char *sig, size_t *sig_len)
{
    if( ctx == NULL || ctx->pk_info == NULL ||
        pk_hashlen_helper( md_alg, &hash_len ) != 0 )
        return( POLARSSL_ERR_PK_BAD_INPUT_DATA );

    if( ctx->pk_info->sign_func == NULL )
        return( POLARSSL_ERR_PK_TYPE_MISMATCH );

    return( ctx->pk_info->sign_func( ctx->pk_ctx, md_alg, hash, hash_len,
                                     sig, sig_len) );
}

/*
 * Decrypt message
 */
int pk_decrypt( pk_context *ctx,
                const unsigned char *input, size_t ilen,
                unsigned char *output, size_t *olen, size_t osize)
{
    if( ctx == NULL || ctx->pk_info == NULL )
        return( POLARSSL_ERR_PK_BAD_INPUT_DATA );

    if( ctx->pk_info->decrypt_func == NULL )
        return( POLARSSL_ERR_PK_TYPE_MISMATCH );

    return( ctx->pk_info->decrypt_func( ctx->pk_ctx, input, ilen,
                output, olen, osize) );
}

/*
 * Encrypt message
 */
int pk_encrypt( pk_context *ctx,
                const unsigned char *input, size_t ilen,
                unsigned char *output, size_t *olen, size_t osize)
{
    if( ctx == NULL || ctx->pk_info == NULL )
        return( POLARSSL_ERR_PK_BAD_INPUT_DATA );

    if( ctx->pk_info->encrypt_func == NULL )
        return( POLARSSL_ERR_PK_TYPE_MISMATCH );

    return( ctx->pk_info->encrypt_func( ctx->pk_ctx, input, ilen,
                output, olen, osize) );
}

/*
 * Access the PK type
 */
pk_type_t pk_get_type( const pk_context *ctx )
{
    if( ctx == NULL || ctx->pk_info == NULL )
        return( POLARSSL_PK_NONE );

    return( ctx->pk_info->type );
}

#endif /* POLARSSL_PK_C */
