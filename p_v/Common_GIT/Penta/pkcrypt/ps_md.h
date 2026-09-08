/**
 * \file md.h
 *
 * \brief Generic message digest wrapper
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
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
#ifndef POLARSSL_MD_H
#define POLARSSL_MD_H

#include <string.h>

#if defined(_MSC_VER) && !defined(inline)
#define inline _inline
#else
#if defined(__ARMCC_VERSION) && !defined(inline)
#define inline __inline
#endif /* __ARMCC_VERSION */
#endif /*_MSC_VER */

#if !defined(inline)
#define inline
#endif

#define POLARSSL_ERR_MD_FEATURE_UNAVAILABLE                -0x5080  /**< The selected feature is not available. */
#define POLARSSL_ERR_MD_BAD_INPUT_DATA                     -0x5100  /**< Bad input parameters to function. */
#define POLARSSL_ERR_MD_ALLOC_FAILED                       -0x5180  /**< Failed to allocate memory. */
#define POLARSSL_ERR_MD_FILE_IO_ERROR                      -0x5200  /**< Opening or reading of file failed. */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POLARSSL_MD_NONE=0,
    POLARSSL_MD_MD2,
    POLARSSL_MD_MD4,
    POLARSSL_MD_MD5,
    POLARSSL_MD_SHA1,
    POLARSSL_MD_SHA224,
    POLARSSL_MD_SHA256,
    POLARSSL_MD_SHA384,
    POLARSSL_MD_SHA512,
    POLARSSL_MD_RIPEMD160,
} md_type_t;

#if defined(POLARSSL_SHA512_C)
#define POLARSSL_MD_MAX_SIZE         64  /* longest known is SHA512 */
#else
#define POLARSSL_MD_MAX_SIZE         32  /* longest known is SHA256 or less */
#endif

/**
 * Message digest information. Allows message digest functions to be called
 * in a generic way.
 */
typedef struct {
    /** Digest identifier */
    md_type_t type;

    /** Name of the message digest */
    const char * name;

    /** Output length of the digest function */
    int size;
} md_info_t;

/**
 * Generic message digest context.
 */
typedef struct {
    /** Information about the associated message digest */
    const md_info_t *md_info;

    /** Digest-specific context */
    void *md_ctx;
} md_context_t;

#define MD_CONTEXT_T_INIT { \
    NULL, /* md_info */ \
    NULL, /* md_ctx */ \
}

/**
 * \brief           Returns the message digest information associated with the
 *                  given digest type.
 *
 * \param md_type   type of digest to search for.
 *
 * \return          The message digest information associated with md_type or
 *                  NULL if not found.
 */
const md_info_t *md_info_from_type( md_type_t md_type );

/**
 * \brief           Returns the size of the message digest output.
 *
 * \param md_info   message digest info
 *
 * \return          size of the message digest output.
 */
static inline unsigned char md_get_size( const md_info_t *md_info )
{
    if( md_info == NULL )
        return( 0 );

    return md_info->size;
}

/**
 * \brief           Returns the type of the message digest output.
 *
 * \param md_info   message digest info
 *
 * \return          type of the message digest output.
 */
static inline md_type_t md_get_type( const md_info_t *md_info )
{
    if( md_info == NULL )
        return( POLARSSL_MD_NONE );

    return md_info->type;
}

/**
 * \brief           Returns the name of the message digest output.
 *
 * \param md_info   message digest info
 *
 * \return          name of the message digest output.
 */
static inline const char *md_get_name( const md_info_t *md_info )
{
    if( md_info == NULL )
        return( NULL );

    return md_info->name;
}

#ifdef __cplusplus
}
#endif

#endif /* POLARSSL_MD_H */
