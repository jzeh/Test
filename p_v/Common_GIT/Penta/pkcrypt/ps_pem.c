/*
 *  Privacy Enhanced Mail (PEM) decoding
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

#if defined(POLARSSL_PEM_PARSE_C) || defined(POLARSSL_PEM_WRITE_C)
#include "ps_pem.h"
#include "ps_base64.h"
#include "ps_cipher.h"

#define polarssl_malloc     malloc
#define polarssl_free       free

#include <stdlib.h>

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#if defined(POLARSSL_PEM_PARSE_C)
void pem_init( pem_context *ctx )
{
    memset( ctx, 0, sizeof( pem_context ) );
}


int pem_read_buffer( pem_context *ctx, const char *header, const char *footer,
                     const unsigned char *data, const unsigned char *pwd,
                     size_t pwdlen, size_t *use_len )
{
    int ret, enc;
    size_t len;
    unsigned char *buf;
    const unsigned char *s1, *s2, *end;
    ((void) pwd);
    ((void) pwdlen);

    if( ctx == NULL )
        return( POLARSSL_ERR_PEM_BAD_INPUT_DATA );

    s1 = (unsigned char *) strstr( (const char *) data, header );

    if( s1 == NULL )
        return( POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT );

    s2 = (unsigned char *) strstr( (const char *) data, footer );

    if( s2 == NULL || s2 <= s1 )
        return( POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT );

    s1 += strlen( header );
    if( *s1 == '\r' ) s1++;
    if( *s1 == '\n' ) s1++;
    else return( POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT );

    end = s2;
    end += strlen( footer );
    if( *end == '\r' ) end++;
    if( *end == '\n' ) end++;
    *use_len = end - data;

    enc = 0;

    if( memcmp( s1, "Proc-Type: 4,ENCRYPTED", 22 ) == 0 )
    {
        return( POLARSSL_ERR_PEM_FEATURE_UNAVAILABLE );
    }

    len = 0;
    ret = base64_decode( NULL, &len, s1, s2 - s1 );

    if( ret == POLARSSL_ERR_BASE64_INVALID_CHARACTER )
        return( POLARSSL_ERR_PEM_INVALID_DATA + ret );

    if( ( buf = (unsigned char *) polarssl_malloc( len ) ) == NULL )
        return( POLARSSL_ERR_PEM_MALLOC_FAILED );

    if( ( ret = base64_decode( buf, &len, s1, s2 - s1 ) ) != 0 )
    {
        polarssl_free( buf );
        return( POLARSSL_ERR_PEM_INVALID_DATA + ret );
    }

    if( enc != 0 )
    {
        polarssl_free( buf );
        return( POLARSSL_ERR_PEM_FEATURE_UNAVAILABLE );
    }

    ctx->buf = buf;
    ctx->buflen = len;

    return( 0 );
}

void pem_free( pem_context *ctx )
{
    polarssl_free( ctx->buf );
    polarssl_free( ctx->info );

    polarssl_zeroize( ctx, sizeof( pem_context ) );
}
#endif /* POLARSSL_PEM_PARSE_C */

#endif /* POLARSSL_PEM_PARSE_C || POLARSSL_PEM_WRITE_C */
