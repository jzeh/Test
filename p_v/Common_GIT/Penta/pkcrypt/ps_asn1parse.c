/*
 *  Generic ASN.1 parsing
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

#if defined(POLARSSL_ASN1_PARSE_C)

#include "ps_asn1.h"

#if defined(POLARSSL_BIGNUM_C)
#include "ps_bignum.h"
#endif

#include <string.h>
#include <stdlib.h>

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 * ASN.1 DER decoding routines
 */
int asn1_get_len( unsigned char **p,
                  const unsigned char *end,
                  size_t *len )
{
    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( ( **p & 0x80 ) == 0 )
        *len = *(*p)++;
    else
    {
        switch( **p & 0x7F )
        {
        case 1:
            if( ( end - *p ) < 2 )
                return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

            *len = (*p)[1];
            (*p) += 2;
            break;

        case 2:
            if( ( end - *p ) < 3 )
                return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

            *len = ( (*p)[1] << 8 ) | (*p)[2];
            (*p) += 3;
            break;

        case 3:
            if( ( end - *p ) < 4 )
                return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

            *len = ( (*p)[1] << 16 ) | ( (*p)[2] << 8 ) | (*p)[3];
            (*p) += 4;
            break;

        case 4:
            if( ( end - *p ) < 5 )
                return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

            *len = ( (*p)[1] << 24 ) | ( (*p)[2] << 16 ) | ( (*p)[3] << 8 ) |
                   (*p)[4];
            (*p) += 5;
            break;

        default:
            return( POLARSSL_ERR_ASN1_INVALID_LENGTH );
        }
    }

    if( *len > (size_t) ( end - *p ) )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

    return( 0 );
}

int asn1_get_tag( unsigned char **p,
                  const unsigned char *end,
                  size_t *len, int tag )
{
    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( **p != tag )
        return( POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    (*p)++;

    return( asn1_get_len( p, end, len ) );
}

int asn1_get_int( unsigned char **p,
                  const unsigned char *end,
                  int *val )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len, ASN1_INTEGER ) ) != 0 )
        return( ret );

    if( len > sizeof( int ) || ( **p & 0x80 ) != 0 )
        return( POLARSSL_ERR_ASN1_INVALID_LENGTH );

    *val = 0;

    while( len-- > 0 )
    {
        *val = ( *val << 8 ) | **p;
        (*p)++;
    }

    return( 0 );
}

#if defined(POLARSSL_BIGNUM_C)
int asn1_get_mpi( unsigned char **p,
                  const unsigned char *end,
                  mpi *X )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len, ASN1_INTEGER ) ) != 0 )
        return( ret );

    ret = mpi_read_binary( X, *p, len );

    *p += len;

    return( ret );
}
#endif /* POLARSSL_BIGNUM_C */

/*
 * Get a bit string without unused bits
 */
int asn1_get_bitstring_null( unsigned char **p, const unsigned char *end,
                             size_t *len )
{
    int ret;

    if( ( ret = asn1_get_tag( p, end, len, ASN1_BIT_STRING ) ) != 0 )
        return( ret );

    if( (*len)-- < 2 || *(*p)++ != 0 )
        return( POLARSSL_ERR_ASN1_INVALID_DATA );

    return( 0 );
}

int asn1_get_alg( unsigned char **p,
                  const unsigned char *end,
                  asn1_buf *alg, asn1_buf *params )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( ret );

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

    alg->tag = **p;
    end = *p + len;

    if( ( ret = asn1_get_tag( p, end, &alg->len, ASN1_OID ) ) != 0 )
        return( ret );

    alg->p = *p;
    *p += alg->len;

    if( *p == end )
    {
        polarssl_zeroize( params, sizeof(asn1_buf) );
        return( 0 );
    }

    params->tag = **p;
    (*p)++;

    if( ( ret = asn1_get_len( p, end, &params->len ) ) != 0 )
        return( ret );

    params->p = *p;
    *p += params->len;

    if( *p != end )
        return( POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}



#endif /* POLARSSL_ASN1_PARSE_C */
