/*------------------------------------------------------------------
 * HAL_safe_types.h
 *
 * March 2007, Bo Berry
 *
 * Copyright (c) 2007-2011 by Cisco Systems, Inc
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */

#ifndef __HAL_SAFE_TYPES_H__
#define __HAL_SAFE_TYPES_H__

#include "HAL_safe_limits.h"


/*
 * Abstract header file for portability.
 */
#if !defined(AT32F435VMT7)

#ifndef TRUE
#define TRUE   ( true )
#endif

#ifndef FALSE
#define FALSE  ( false )
#endif
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif


#ifndef BOOL
typedef bool                    BOOL;
#endif

#ifndef U8
typedef unsigned char           U8;
#endif
#ifndef U16
typedef unsigned short          U16;
#endif
#ifndef U32
typedef unsigned int            U32;
#endif
#ifndef U64
typedef unsigned long long      U64;
#endif
#ifndef VU8
typedef volatile unsigned char  VU8;
#endif
#ifndef VU16
typedef volatile unsigned short VU16;
#endif
#ifndef VU32
typedef volatile unsigned int   VU32;
#endif
#ifndef S8
typedef signed  char            S8;
#endif
#ifndef S16
typedef signed short            S16;
#endif
#ifndef S32
typedef signed int              S32;
#endif

#ifndef boolean_t
typedef unsigned char boolean_t;
#endif

#ifndef int8_t
typedef signed char int8_t;
#endif

#ifndef int16_t
typedef short int16_t;
#endif

#ifndef int32_t
typedef int int32_t;
#endif

#ifndef uchar_t
typedef unsigned char uchar_t;
#endif

#ifndef  uint8_t
typedef unsigned char uint8_t;
#endif

#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif

#ifndef u8
typedef U8 u8;
#endif

#ifndef INT8U
typedef U8 INT8U;
#endif

#ifndef INT8S
typedef S8 INT8S;
#endif

#ifndef u16
typedef U16 u16;
#endif

#ifndef INT16U
typedef U16 INT16U;
#endif

#ifndef INT32U
typedef U32 INT32U;
#endif

#ifndef u32
typedef U32 u32;
#endif
#ifndef INT64U
typedef U64 INT64U;
#endif

#ifndef INT16S
typedef S16 INT16S;
#endif
#ifndef s16
typedef S16 s16;
#endif
#ifndef INT32S
typedef S32 INT32S;
#endif


/* These types must be 16-bit, 32-bit or larger integer */
typedef unsigned int            UINT;
/* These types must be 8-bit integer */
typedef unsigned char           UCHAR;
typedef unsigned char           BYTE;

/* These types must be 16-bit integer */
typedef unsigned short          USHORT;
typedef unsigned short          WORD;
typedef unsigned short          WCHAR;

/* These types must be 32-bit integer */
/*typedef long long;*/
typedef unsigned long           ULONG;
typedef unsigned long           DWORD;
    
    

#if 0
#if POINTER_BIT == 64
#ifndef intptr_t;
typedef long long intptr_t;
#endif

#ifndef uintptr_t;
typedef unsigned long long uintptr_t;
#endif

#else

#ifndef intptr_t;
typedef signed int intptr_t;
#endif
#ifndef uintptr_t;
typedef unsigned int uintptr_t;
#endif

#endif
#endif


#ifndef ushort
typedef unsigned short ushort;
#endif

#ifndef int_t
typedef int int_t;
#endif

#ifndef uint_t
typedef unsigned int uint_t;
#endif

#ifndef ulong
typedef unsigned long ulong;
#endif

#ifndef ulonglong
typedef unsigned long long ullong;
#endif


#endif /* __HAL_SAFE_TYPES_H__ */

