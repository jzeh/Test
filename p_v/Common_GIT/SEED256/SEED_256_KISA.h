/*******************************************************************************
*
* FILE:         SEED_256_KISA.h
*
* DESCRIPTION:  header file for SEED_256_KISA.c
*
*******************************************************************************/

#ifndef _SEED_256_KISA_H
#define _SEED_256_KISA_H

/********************** Include files ************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/********************* Type Definitions **********************/

#ifndef TYPE_DEFINITION
    #define TYPE_DEFINITION
    #if defined(__alpha)
        typedef unsigned int        DWORD;
        typedef unsigned short      WORD;
    #else
        typedef unsigned long int   DWORD;
        typedef unsigned short int  WORD;
    #endif
    typedef unsigned char           BYTE;
#endif

/***************************** Endianness Define **************/
// If endianness is not defined correctly, you must modify here.
// SEED uses the Little endian as a defalut order

#define SEED_LITTLE_ENDIAN
//#define SEED_BIG_ENDIAN

/******************* Constant Definitions *********************/

#define NoRounds         24
#define NoRoundKeys      (NoRounds*2)
#define SeedBlockSize    16    /* in bytes */
#define SeedBlockLen     128   /* in bits */


/********************** Common Macros ************************/

#if defined(_MSC_VER)
    #define ROTL(x, n)     (_lrotl((x), (n)))
    #define ROTR(x, n)     (_lrotr((x), (n)))
#else
    #define ROTL(x, n)     (((x) << (n)) | ((x) >> (32-(n))))
    #define ROTR(x, n)     (((x) >> (n)) | ((x) << (32-(n))))
#endif


/**************** Function Prototype Declarations **************/

/* encryption function */
// BYTE *pbData: 				[in,out]	data to be encrypted
// DWORD *pdwRoundKey: 	[in]			round keys for encryption
void SeedEncrypt(BYTE *pbData, DWORD *pdwRoundKey);

/* decryption function */
// BYTE *pbData: 				[in,out]	data to be decrypted
// DWORD *pdwRoundKey: 	[in]			round keys for decryption
void SeedDecrypt(BYTE *pbData, DWORD *pdwRoundKey);

/* key scheduling function */
// DWORD *pdwRoundKey: 	[out]		round keys for encryption or decryption
// BYTE *pbUserKey: 		[in]			secret user key
void SeedRoundKey(DWORD *pdwRoundKey, BYTE *pbUserKey);

/*************************** END OF FILE **************************************/
#endif
