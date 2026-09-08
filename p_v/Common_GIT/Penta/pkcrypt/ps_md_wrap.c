/**
 * \file md_wrap.c

 * \brief Generic message digest wrapper for mbed TLS
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

#if !defined(POLARSSL_CONFIG_FILE)
#include "ps_config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_MD_C)

#include "ps_md_wrap.h"

#include <stdlib.h>

/* Implementation that should never be optimized out by the compiler */

#if false
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}
#endif

/*
 * Wrappers for generic message digests
 */
#if defined(POLARSSL_SHA256_C)
const md_info_t sha256_info = {
    POLARSSL_MD_SHA256,
    "SHA256",
    32
};

#endif /* POLARSSL_SHA256_C */

#endif /* POLARSSL_MD_C */
