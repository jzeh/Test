#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hight.h"
#ifdef DUKPT_SERVER
	#include "dukpt_server_module.h"
#else
    #include "dukpt.h"
#endif

static BYTE Delta[128] = 
{
	0x5A,0x6D,0x36,0x1B,0x0D,0x06,0x03,0x41,
	0x60,0x30,0x18,0x4C,0x66,0x33,0x59,0x2C,
	0x56,0x2B,0x15,0x4A,0x65,0x72,0x39,0x1C,
	0x4E,0x67,0x73,0x79,0x3C,0x5E,0x6F,0x37,
	0x5B,0x2D,0x16,0x0B,0x05,0x42,0x21,0x50,
	0x28,0x54,0x2A,0x55,0x6A,0x75,0x7A,0x7D,
	0x3E,0x5F,0x2F,0x17,0x4B,0x25,0x52,0x29,
	0x14,0x0A,0x45,0x62,0x31,0x58,0x6C,0x76,
	0x3B,0x1D,0x0E,0x47,0x63,0x71,0x78,0x7C,
	0x7E,0x7F,0x3F,0x1F,0x0F,0x07,0x43,0x61,
	0x70,0x38,0x5C,0x6E,0x77,0x7B,0x3D,0x1E,
	0x4F,0x27,0x53,0x69,0x34,0x1A,0x4D,0x26,
	0x13,0x49,0x24,0x12,0x09,0x04,0x02,0x01,
	0x40,0x20,0x10,0x08,0x44,0x22,0x11,0x48,
	0x64,0x32,0x19,0x0C,0x46,0x23,0x51,0x68,
	0x74,0x3A,0x5D,0x2E,0x57,0x6B,0x35,0x5A
};

static BYTE F0[256] = 
{
	0x00,0x86,0x0D,0x8B,0x1A,0x9C,0x17,0x91,
	0x34,0xB2,0x39,0xBF,0x2E,0xA8,0x23,0xA5,
	0x68,0xEE,0x65,0xE3,0x72,0xF4,0x7F,0xF9,
	0x5C,0xDA,0x51,0xD7,0x46,0xC0,0x4B,0xCD,
	0xD0,0x56,0xDD,0x5B,0xCA,0x4C,0xC7,0x41,
	0xE4,0x62,0xE9,0x6F,0xFE,0x78,0xF3,0x75,
	0xB8,0x3E,0xB5,0x33,0xA2,0x24,0xAF,0x29,
	0x8C,0x0A,0x81,0x07,0x96,0x10,0x9B,0x1D,
	0xA1,0x27,0xAC,0x2A,0xBB,0x3D,0xB6,0x30,
	0x95,0x13,0x98,0x1E,0x8F,0x09,0x82,0x04,
	0xC9,0x4F,0xC4,0x42,0xD3,0x55,0xDE,0x58,
	0xFD,0x7B,0xF0,0x76,0xE7,0x61,0xEA,0x6C,
	0x71,0xF7,0x7C,0xFA,0x6B,0xED,0x66,0xE0,
	0x45,0xC3,0x48,0xCE,0x5F,0xD9,0x52,0xD4,
	0x19,0x9F,0x14,0x92,0x03,0x85,0x0E,0x88,
	0x2D,0xAB,0x20,0xA6,0x37,0xB1,0x3A,0xBC,
	0x43,0xC5,0x4E,0xC8,0x59,0xDF,0x54,0xD2,
	0x77,0xF1,0x7A,0xFC,0x6D,0xEB,0x60,0xE6,
	0x2B,0xAD,0x26,0xA0,0x31,0xB7,0x3C,0xBA,
	0x1F,0x99,0x12,0x94,0x05,0x83,0x08,0x8E,
	0x93,0x15,0x9E,0x18,0x89,0x0F,0x84,0x02,
	0xA7,0x21,0xAA,0x2C,0xBD,0x3B,0xB0,0x36,
	0xFB,0x7D,0xF6,0x70,0xE1,0x67,0xEC,0x6A,
	0xCF,0x49,0xC2,0x44,0xD5,0x53,0xD8,0x5E,
	0xE2,0x64,0xEF,0x69,0xF8,0x7E,0xF5,0x73,
	0xD6,0x50,0xDB,0x5D,0xCC,0x4A,0xC1,0x47,
	0x8A,0x0C,0x87,0x01,0x90,0x16,0x9D,0x1B,
	0xBE,0x38,0xB3,0x35,0xA4,0x22,0xA9,0x2F,
	0x32,0xB4,0x3F,0xB9,0x28,0xAE,0x25,0xA3,
	0x06,0x80,0x0B,0x8D,0x1C,0x9A,0x11,0x97,
	0x5A,0xDC,0x57,0xD1,0x40,0xC6,0x4D,0xCB,
	0x6E,0xE8,0x63,0xE5,0x74,0xF2,0x79,0xFF
};

static BYTE F1[256] = 
{
	0x00,0x58,0xB0,0xE8,0x61,0x39,0xD1,0x89,
	0xC2,0x9A,0x72,0x2A,0xA3,0xFB,0x13,0x4B,
	0x85,0xDD,0x35,0x6D,0xE4,0xBC,0x54,0x0C,
	0x47,0x1F,0xF7,0xAF,0x26,0x7E,0x96,0xCE,
	0x0B,0x53,0xBB,0xE3,0x6A,0x32,0xDA,0x82,
	0xC9,0x91,0x79,0x21,0xA8,0xF0,0x18,0x40,
	0x8E,0xD6,0x3E,0x66,0xEF,0xB7,0x5F,0x07,
	0x4C,0x14,0xFC,0xA4,0x2D,0x75,0x9D,0xC5,
	0x16,0x4E,0xA6,0xFE,0x77,0x2F,0xC7,0x9F,
	0xD4,0x8C,0x64,0x3C,0xB5,0xED,0x05,0x5D,
	0x93,0xCB,0x23,0x7B,0xF2,0xAA,0x42,0x1A,
	0x51,0x09,0xE1,0xB9,0x30,0x68,0x80,0xD8,
	0x1D,0x45,0xAD,0xF5,0x7C,0x24,0xCC,0x94,
	0xDF,0x87,0x6F,0x37,0xBE,0xE6,0x0E,0x56,
	0x98,0xC0,0x28,0x70,0xF9,0xA1,0x49,0x11,
	0x5A,0x02,0xEA,0xB2,0x3B,0x63,0x8B,0xD3,
	0x2C,0x74,0x9C,0xC4,0x4D,0x15,0xFD,0xA5,
	0xEE,0xB6,0x5E,0x06,0x8F,0xD7,0x3F,0x67,
	0xA9,0xF1,0x19,0x41,0xC8,0x90,0x78,0x20,
	0x6B,0x33,0xDB,0x83,0x0A,0x52,0xBA,0xE2,
	0x27,0x7F,0x97,0xCF,0x46,0x1E,0xF6,0xAE,
	0xE5,0xBD,0x55,0x0D,0x84,0xDC,0x34,0x6C,
	0xA2,0xFA,0x12,0x4A,0xC3,0x9B,0x73,0x2B,
	0x60,0x38,0xD0,0x88,0x01,0x59,0xB1,0xE9,
	0x3A,0x62,0x8A,0xD2,0x5B,0x03,0xEB,0xB3,
	0xF8,0xA0,0x48,0x10,0x99,0xC1,0x29,0x71,
	0xBF,0xE7,0x0F,0x57,0xDE,0x86,0x6E,0x36,
	0x7D,0x25,0xCD,0x95,0x1C,0x44,0xAC,0xF4,
	0x31,0x69,0x81,0xD9,0x50,0x08,0xE0,0xB8,
	0xF3,0xAB,0x43,0x1B,0x92,0xCA,0x22,0x7A,
	0xB4,0xEC,0x04,0x5C,0xD5,0x8D,0x65,0x3D,
	0x76,0x2E,0xC6,0x9E,0x17,0x4F,0xA7,0xFF
};

static BYTE HIGHT_F0[256] = {
        0x00,0x86,0x0D,0x8B,0x1A,0x9C,0x17,0x91,
        0x34,0xB2,0x39,0xBF,0x2E,0xA8,0x23,0xA5,
        0x68,0xEE,0x65,0xE3,0x72,0xF4,0x7F,0xF9,
        0x5C,0xDA,0x51,0xD7,0x46,0xC0,0x4B,0xCD,
        0xD0,0x56,0xDD,0x5B,0xCA,0x4C,0xC7,0x41,
        0xE4,0x62,0xE9,0x6F,0xFE,0x78,0xF3,0x75,
        0xB8,0x3E,0xB5,0x33,0xA2,0x24,0xAF,0x29,
        0x8C,0x0A,0x81,0x07,0x96,0x10,0x9B,0x1D,
        0xA1,0x27,0xAC,0x2A,0xBB,0x3D,0xB6,0x30,
        0x95,0x13,0x98,0x1E,0x8F,0x09,0x82,0x04,
        0xC9,0x4F,0xC4,0x42,0xD3,0x55,0xDE,0x58,
        0xFD,0x7B,0xF0,0x76,0xE7,0x61,0xEA,0x6C,
        0x71,0xF7,0x7C,0xFA,0x6B,0xED,0x66,0xE0,
        0x45,0xC3,0x48,0xCE,0x5F,0xD9,0x52,0xD4,
        0x19,0x9F,0x14,0x92,0x03,0x85,0x0E,0x88,
        0x2D,0xAB,0x20,0xA6,0x37,0xB1,0x3A,0xBC,
        0x43,0xC5,0x4E,0xC8,0x59,0xDF,0x54,0xD2,
        0x77,0xF1,0x7A,0xFC,0x6D,0xEB,0x60,0xE6,
        0x2B,0xAD,0x26,0xA0,0x31,0xB7,0x3C,0xBA,
        0x1F,0x99,0x12,0x94,0x05,0x83,0x08,0x8E,
        0x93,0x15,0x9E,0x18,0x89,0x0F,0x84,0x02,
        0xA7,0x21,0xAA,0x2C,0xBD,0x3B,0xB0,0x36,
        0xFB,0x7D,0xF6,0x70,0xE1,0x67,0xEC,0x6A,
        0xCF,0x49,0xC2,0x44,0xD5,0x53,0xD8,0x5E,
        0xE2,0x64,0xEF,0x69,0xF8,0x7E,0xF5,0x73,
        0xD6,0x50,0xDB,0x5D,0xCC,0x4A,0xC1,0x47,
        0x8A,0x0C,0x87,0x01,0x90,0x16,0x9D,0x1B,
        0xBE,0x38,0xB3,0x35,0xA4,0x22,0xA9,0x2F,
        0x32,0xB4,0x3F,0xB9,0x28,0xAE,0x25,0xA3,
        0x06,0x80,0x0B,0x8D,0x1C,0x9A,0x11,0x97,
        0x5A,0xDC,0x57,0xD1,0x40,0xC6,0x4D,0xCB,
        0x6E,0xE8,0x63,0xE5,0x74,0xF2,0x79,0xFF};

static BYTE HIGHT_F1[256] = {
        0x00,0x58,0xB0,0xE8,0x61,0x39,0xD1,0x89,
        0xC2,0x9A,0x72,0x2A,0xA3,0xFB,0x13,0x4B,
        0x85,0xDD,0x35,0x6D,0xE4,0xBC,0x54,0x0C,
        0x47,0x1F,0xF7,0xAF,0x26,0x7E,0x96,0xCE,
        0x0B,0x53,0xBB,0xE3,0x6A,0x32,0xDA,0x82,
        0xC9,0x91,0x79,0x21,0xA8,0xF0,0x18,0x40,
        0x8E,0xD6,0x3E,0x66,0xEF,0xB7,0x5F,0x07,
        0x4C,0x14,0xFC,0xA4,0x2D,0x75,0x9D,0xC5,
        0x16,0x4E,0xA6,0xFE,0x77,0x2F,0xC7,0x9F,
        0xD4,0x8C,0x64,0x3C,0xB5,0xED,0x05,0x5D,
        0x93,0xCB,0x23,0x7B,0xF2,0xAA,0x42,0x1A,
        0x51,0x09,0xE1,0xB9,0x30,0x68,0x80,0xD8,
        0x1D,0x45,0xAD,0xF5,0x7C,0x24,0xCC,0x94,
        0xDF,0x87,0x6F,0x37,0xBE,0xE6,0x0E,0x56,
        0x98,0xC0,0x28,0x70,0xF9,0xA1,0x49,0x11,
        0x5A,0x02,0xEA,0xB2,0x3B,0x63,0x8B,0xD3,
        0x2C,0x74,0x9C,0xC4,0x4D,0x15,0xFD,0xA5,
        0xEE,0xB6,0x5E,0x06,0x8F,0xD7,0x3F,0x67,
        0xA9,0xF1,0x19,0x41,0xC8,0x90,0x78,0x20,
        0x6B,0x33,0xDB,0x83,0x0A,0x52,0xBA,0xE2,
        0x27,0x7F,0x97,0xCF,0x46,0x1E,0xF6,0xAE,
        0xE5,0xBD,0x55,0x0D,0x84,0xDC,0x34,0x6C,
        0xA2,0xFA,0x12,0x4A,0xC3,0x9B,0x73,0x2B,
        0x60,0x38,0xD0,0x88,0x01,0x59,0xB1,0xE9,
        0x3A,0x62,0x8A,0xD2,0x5B,0x03,0xEB,0xB3,
        0xF8,0xA0,0x48,0x10,0x99,0xC1,0x29,0x71,
        0xBF,0xE7,0x0F,0x57,0xDE,0x86,0x6E,0x36,
        0x7D,0x25,0xCD,0x95,0x1C,0x44,0xAC,0xF4,
        0x31,0x69,0x81,0xD9,0x50,0x08,0xE0,0xB8,
        0xF3,0xAB,0x43,0x1B,0x92,0xCA,0x22,0x7A,
        0xB4,0xEC,0x04,0x5C,0xD5,0x8D,0x65,0x3D,
        0x76,0x2E,0xC6,0x9E,0x17,0x4F,0xA7,0xFF};

#define BLOCK_SIZE_HIGHT		8
#define BLOCK_SIZE_HIGHT_INT	2

#define BLOCK_XOR_HIGHT( OUT_VALUE, IN_VALUE1, IN_VALUE2 ) {	\
	OUT_VALUE[0] = IN_VALUE1[0] ^ IN_VALUE2[0];			\
	OUT_VALUE[1] = IN_VALUE1[1] ^ IN_VALUE2[1];			\
}														\

#define PADDING_ENC_PROCESS_HIGHT( INOUT_VALUE, IN_START, IN_MAX, PADDING_VALUE ){	\
	int i;													\
	for( i =0 ; i<IN_MAX; i++ )								\
		INOUT_VALUE[IN_START+i] = PADDING_VALUE;			\
}

#define PADDING_DEC_PROCESS_HIGHT( INOUT_VALUE, PADDING_VALUE ){	\
	int i;													\
	for( i =BLOCK_SIZE_HIGHT ; i>-1; --i )								\
	{														\
		if( PADDING_VALUE == INOUT_VALUE[i])				\
			INOUT_VALUE[i] = 0x00;							\
		else												\
			break;											\
	}														\
}

// Encryption Round 
#define HIGHT_ENC(k, i0,i1,i2,i3,i4,i5,i6,i7) {                         \
	XX[i0] = (XX[i0] ^ (HIGHT_F0[XX[i1]] + RoundKey[4*k+3])) & 0xFF;    \
	XX[i2] = (XX[i2] + (HIGHT_F1[XX[i3]] ^ RoundKey[4*k+2])) & 0xFF;    \
	XX[i4] = (XX[i4] ^ (HIGHT_F0[XX[i5]] + RoundKey[4*k+1])) & 0xFF;    \
	XX[i6] = (XX[i6] + (HIGHT_F1[XX[i7]] ^ RoundKey[4*k+0])) & 0xFF;    \
	}

#define EncIni_Transformation(x0,x2,x4,x6,mk0,mk1,mk2,mk3)	\
	t0 = x0 + mk0;										\
	t2 = x2 ^ mk1;										\
	t4 = x4 + mk2;										\
	t6 = x6 ^ mk3; 

#define EncFin_Transformation(x0,x2,x4,x6,mk0,mk1,mk2,mk3)	\
	out[0] = x0 + mk0;										\
	out[2] = x2 ^ mk1;										\
	out[4] = x4 + mk2;										\
	out[6] = x6 ^ mk3; 

#define Round(x7,x6,x5,x4,x3,x2,x1,x0)				\
	x1 += (F1[x0] ^ key[0]);				\
	x3 ^= (F0[x2] + key[1]);				\
	x5 += (F1[x4] ^ key[2]);				\
	x7 ^= (F0[x6] + key[3]);


void    HIGHT_Encrypt(
            BYTE    *RoundKey,      
            BYTE    *Data)          
                                    
{
    DWORD   XX[8];

    // First Round
    XX[1] = Data[1];
    XX[3] = Data[3];
    XX[5] = Data[5];
    XX[7] = Data[7];

    XX[0] = (Data[0] + RoundKey[0]) & 0xFF;
    XX[2] = (Data[2] ^ RoundKey[1]);
    XX[4] = (Data[4] + RoundKey[2]) & 0xFF;
    XX[6] = (Data[6] ^ RoundKey[3]);

    // Encryption Round 
    #define HIGHT_ENC(k, i0,i1,i2,i3,i4,i5,i6,i7) {                         \
        XX[i0] = (XX[i0] ^ (HIGHT_F0[XX[i1]] + RoundKey[4*k+3])) & 0xFF;    \
        XX[i2] = (XX[i2] + (HIGHT_F1[XX[i3]] ^ RoundKey[4*k+2])) & 0xFF;    \
        XX[i4] = (XX[i4] ^ (HIGHT_F0[XX[i5]] + RoundKey[4*k+1])) & 0xFF;    \
        XX[i6] = (XX[i6] + (HIGHT_F1[XX[i7]] ^ RoundKey[4*k+0])) & 0xFF;    \
    }

	HIGHT_ENC( 2,  7,6,5,4,3,2,1,0);
    HIGHT_ENC( 3,  6,5,4,3,2,1,0,7);
    HIGHT_ENC( 4,  5,4,3,2,1,0,7,6);
    HIGHT_ENC( 5,  4,3,2,1,0,7,6,5);
    HIGHT_ENC( 6,  3,2,1,0,7,6,5,4);
    HIGHT_ENC( 7,  2,1,0,7,6,5,4,3);
    HIGHT_ENC( 8,  1,0,7,6,5,4,3,2);
    HIGHT_ENC( 9,  0,7,6,5,4,3,2,1);
    HIGHT_ENC(10,  7,6,5,4,3,2,1,0);
    HIGHT_ENC(11,  6,5,4,3,2,1,0,7);
    HIGHT_ENC(12,  5,4,3,2,1,0,7,6);
    HIGHT_ENC(13,  4,3,2,1,0,7,6,5);
    HIGHT_ENC(14,  3,2,1,0,7,6,5,4);
    HIGHT_ENC(15,  2,1,0,7,6,5,4,3);
    HIGHT_ENC(16,  1,0,7,6,5,4,3,2);
    HIGHT_ENC(17,  0,7,6,5,4,3,2,1);
    HIGHT_ENC(18,  7,6,5,4,3,2,1,0);
    HIGHT_ENC(19,  6,5,4,3,2,1,0,7);
    HIGHT_ENC(20,  5,4,3,2,1,0,7,6);
    HIGHT_ENC(21,  4,3,2,1,0,7,6,5);
    HIGHT_ENC(22,  3,2,1,0,7,6,5,4);
    HIGHT_ENC(23,  2,1,0,7,6,5,4,3);
    HIGHT_ENC(24,  1,0,7,6,5,4,3,2);
    HIGHT_ENC(25,  0,7,6,5,4,3,2,1);
    HIGHT_ENC(26,  7,6,5,4,3,2,1,0);
    HIGHT_ENC(27,  6,5,4,3,2,1,0,7);
    HIGHT_ENC(28,  5,4,3,2,1,0,7,6);
    HIGHT_ENC(29,  4,3,2,1,0,7,6,5);
    HIGHT_ENC(30,  3,2,1,0,7,6,5,4);
    HIGHT_ENC(31,  2,1,0,7,6,5,4,3);
    HIGHT_ENC(32,  1,0,7,6,5,4,3,2);
    HIGHT_ENC(33,  0,7,6,5,4,3,2,1);

    // Final Round
    Data[1] = (BYTE) XX[2];
    Data[3] = (BYTE) XX[4];
    Data[5] = (BYTE) XX[6];
    Data[7] = (BYTE) XX[0];

    Data[0] = (BYTE) (XX[1] + RoundKey[4]);
    Data[2] = (BYTE) (XX[3] ^ RoundKey[5]);
    Data[4] = (BYTE) (XX[5] + RoundKey[6]);
    Data[6] = (BYTE) (XX[7] ^ RoundKey[7]);
}

/***************Decryption *************************************************/

// Same as encrypt, except that round keys are applied in reverse order

void    HIGHT_Decrypt(
            BYTE    *RoundKey,     
            BYTE    *Data)          
{
    DWORD   XX[8];

    XX[2] = (BYTE) Data[1];
    XX[4] = (BYTE) Data[3];
    XX[6] = (BYTE) Data[5];
    XX[0] = (BYTE) Data[7];

    XX[1] = (BYTE) (Data[0] - RoundKey[4]);
    XX[3] = (BYTE) (Data[2] ^ RoundKey[5]);
    XX[5] = (BYTE) (Data[4] - RoundKey[6]);
    XX[7] = (BYTE) (Data[6] ^ RoundKey[7]);

    #define HIGHT_DEC(k, i0,i1,i2,i3,i4,i5,i6,i7) {                         \
        XX[i1] = (XX[i1] - (HIGHT_F1[XX[i2]] ^ RoundKey[4*k+2])) & 0xFF;    \
        XX[i3] = (XX[i3] ^ (HIGHT_F0[XX[i4]] + RoundKey[4*k+1])) & 0xFF;    \
        XX[i5] = (XX[i5] - (HIGHT_F1[XX[i6]] ^ RoundKey[4*k+0])) & 0xFF;    \
        XX[i7] = (XX[i7] ^ (HIGHT_F0[XX[i0]] + RoundKey[4*k+3])) & 0xFF;    \
    }

    HIGHT_DEC(33,  7,6,5,4,3,2,1,0);
    HIGHT_DEC(32,  0,7,6,5,4,3,2,1);
    HIGHT_DEC(31,  1,0,7,6,5,4,3,2);
    HIGHT_DEC(30,  2,1,0,7,6,5,4,3);
    HIGHT_DEC(29,  3,2,1,0,7,6,5,4);
    HIGHT_DEC(28,  4,3,2,1,0,7,6,5);
    HIGHT_DEC(27,  5,4,3,2,1,0,7,6);
    HIGHT_DEC(26,  6,5,4,3,2,1,0,7);
    HIGHT_DEC(25,  7,6,5,4,3,2,1,0);
    HIGHT_DEC(24,  0,7,6,5,4,3,2,1);
    HIGHT_DEC(23,  1,0,7,6,5,4,3,2);
    HIGHT_DEC(22,  2,1,0,7,6,5,4,3);
    HIGHT_DEC(21,  3,2,1,0,7,6,5,4);
    HIGHT_DEC(20,  4,3,2,1,0,7,6,5);
    HIGHT_DEC(19,  5,4,3,2,1,0,7,6);
    HIGHT_DEC(18,  6,5,4,3,2,1,0,7);
    HIGHT_DEC(17,  7,6,5,4,3,2,1,0);
    HIGHT_DEC(16,  0,7,6,5,4,3,2,1);
    HIGHT_DEC(15,  1,0,7,6,5,4,3,2);
    HIGHT_DEC(14,  2,1,0,7,6,5,4,3);
    HIGHT_DEC(13,  3,2,1,0,7,6,5,4);
    HIGHT_DEC(12,  4,3,2,1,0,7,6,5);
    HIGHT_DEC(11,  5,4,3,2,1,0,7,6);
    HIGHT_DEC(10,  6,5,4,3,2,1,0,7);
    HIGHT_DEC( 9,  7,6,5,4,3,2,1,0);
    HIGHT_DEC( 8,  0,7,6,5,4,3,2,1);
    HIGHT_DEC( 7,  1,0,7,6,5,4,3,2);
    HIGHT_DEC( 6,  2,1,0,7,6,5,4,3);
    HIGHT_DEC( 5,  3,2,1,0,7,6,5,4);
    HIGHT_DEC( 4,  4,3,2,1,0,7,6,5);
    HIGHT_DEC( 3,  5,4,3,2,1,0,7,6);
    HIGHT_DEC( 2,  6,5,4,3,2,1,0,7);

    Data[1] = (BYTE) (XX[1]);
    Data[3] = (BYTE) (XX[3]);
    Data[5] = (BYTE) (XX[5]);
    Data[7] = (BYTE) (XX[7]);

    Data[0] = (BYTE) (XX[0] - RoundKey[0]);
    Data[2] = (BYTE) (XX[2] ^ RoundKey[1]);
    Data[4] = (BYTE) (XX[4] - RoundKey[2]);
    Data[6] = (BYTE) (XX[6] ^ RoundKey[3]);
}

/*************** END OF FILE **********************************************/


void KISA_HIGHT_ECB_encrypt_forCBC( unsigned char *pbszIN_Key128, unsigned char *pbszUserKey, const unsigned char *in, unsigned char *out )
{
	register unsigned char t0, t1, t2, t3, t4, t5, t6, t7;
	BYTE *key, *key2;

	key = pbszIN_Key128;
	key2 = pbszUserKey;

	t1 = in[1]; t3 = in[3]; t5 = in[5]; t7 = in[7];
	EncIni_Transformation( in[0], in[2], in[4], in[6], key2[12], key2[13], key2[14], key2[15] );

	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 1
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 2
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 3
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 4
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 5
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 6
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 7
	Round(t0,t7,t6,t5,t4,t3,t2,t1);key += 4;		// 8
	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 9
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 10
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 11
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 12
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 13
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 14
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 15
	Round(t0,t7,t6,t5,t4,t3,t2,t1);key += 4;		// 16
	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 17
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 18
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 19
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 20
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 21
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 22
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 23
	Round(t0,t7,t6,t5,t4,t3,t2,t1);key += 4;		// 24
	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 25
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 26
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 27
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 28
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 29
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 30
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 31
	Round(t0,t7,t6,t5,t4,t3,t2,t1);					// 32

	EncFin_Transformation( t1, t3, t5, t7, key2[0], key2[1], key2[2], key2[3] );

	out[1] = t2; out[3] = t4; out[5] = t6; out[7] = t0;
}

void KISA_HIGHT_ECB_encrypt_forCFB( unsigned char *pbszIN_Key128, unsigned char *pbszUserKey, unsigned char *in, unsigned char *out)
{
	register unsigned char t0, t1, t2, t3, t4, t5, t6, t7;
	BYTE *key, *key2;

	key = pbszIN_Key128;
	key2 = pbszUserKey;

	t1 = in[1]; t3 = in[3]; t5 = in[5]; t7 = in[7];
	EncIni_Transformation( in[0], in[2], in[4], in[6], key2[12], key2[13], key2[14], key2[15] );

	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 1
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 2
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 3
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 4
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 5
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 6
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 7
	Round(t0,t7,t6,t5,t4,t3,t2,t1);key += 4;		// 8
	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 9
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 10
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 11
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 12
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 13
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 14
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 15
	Round(t0,t7,t6,t5,t4,t3,t2,t1);key += 4;		// 16
	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 17
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 18
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 19
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 20
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 21
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 22
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 23
	Round(t0,t7,t6,t5,t4,t3,t2,t1);key += 4;		// 24
	Round(t7,t6,t5,t4,t3,t2,t1,t0);key += 4;		// 25
	Round(t6,t5,t4,t3,t2,t1,t0,t7);key += 4;		// 26
	Round(t5,t4,t3,t2,t1,t0,t7,t6);key += 4;		// 27
	Round(t4,t3,t2,t1,t0,t7,t6,t5);key += 4;		// 28
	Round(t3,t2,t1,t0,t7,t6,t5,t4);key += 4;		// 29
	Round(t2,t1,t0,t7,t6,t5,t4,t3);key += 4;		// 30
	Round(t1,t0,t7,t6,t5,t4,t3,t2);key += 4;		// 31
	Round(t0,t7,t6,t5,t4,t3,t2,t1);					// 32

	EncFin_Transformation( t1, t3, t5, t7, key2[0], key2[1], key2[2], key2[3] );

	out[1] = t2; out[3] = t4; out[5] = t6; out[7] = t0;
	memcpy(in, out, 16);
}

#define HIGHT_DEC(k, i0,i1,i2,i3,i4,i5,i6,i7) {                         \
	XX[i1] = (XX[i1] - (HIGHT_F1[XX[i2]] ^ RoundKey[4*k+2])) & 0xFF;    \
	XX[i3] = (XX[i3] ^ (HIGHT_F0[XX[i4]] + RoundKey[4*k+1])) & 0xFF;    \
	XX[i5] = (XX[i5] - (HIGHT_F1[XX[i6]] ^ RoundKey[4*k+0])) & 0xFF;    \
	XX[i7] = (XX[i7] ^ (HIGHT_F0[XX[i0]] + RoundKey[4*k+3])) & 0xFF;    \
}

#define DecIni_Transformation(x0,x2,x4,x6,mk0,mk1,mk2,mk3)	\
	t0 = x0 - mk0;										\
	t2 = x2 ^ mk1;										\
	t4 = x4 - mk2;										\
	t6 = x6 ^ mk3; 

#define DecFin_Transformation(x0,x2,x4,x6,mk0,mk1,mk2,mk3)	\
	out[0] = x0 - mk0;										\
	out[2] = x2 ^ mk1;										\
	out[4] = x4 - mk2;										\
	out[6] = x6 ^ mk3; 


#define DRound(x7,x6,x5,x4,x3,x2,x1,x0)				\
	x1 = x1 - (F1[x0] ^ key[0]);				\
	x3 = x3 ^ (F0[x2] + key[1]);				\
	x5 = x5 - (F1[x4] ^ key[2]);				\
	x7 = x7 ^ (F0[x6] + key[3]); 

void KISA_HIGHT_ECB_decrypt_forCBC( unsigned char *pbszIN_Key128, unsigned char *pbszUserKey, const unsigned char *in, unsigned char *out )
{
	register unsigned char t0, t1, t2, t3, t4, t5, t6, t7;
	unsigned char *key, *key2;

	key = &(pbszIN_Key128[124]);
	key2 = pbszUserKey;

	t1 = in[1]; t3 = in[3]; t5 = in[5]; t7 = in[7];
	DecIni_Transformation( in[0], in[2], in[4], in[6], key2[0], key2[1], key2[2], key2[3] );

	DRound(t7,t6,t5,t4,t3,t2,t1,t0);key -= 4;
	DRound(t0,t7,t6,t5,t4,t3,t2,t1);key -= 4;
	DRound(t1,t0,t7,t6,t5,t4,t3,t2);key -= 4;
	DRound(t2,t1,t0,t7,t6,t5,t4,t3);key -= 4;
	DRound(t3,t2,t1,t0,t7,t6,t5,t4);key -= 4;
	DRound(t4,t3,t2,t1,t0,t7,t6,t5);key -= 4;
	DRound(t5,t4,t3,t2,t1,t0,t7,t6);key -= 4;
	DRound(t6,t5,t4,t3,t2,t1,t0,t7);key -= 4;
	DRound(t7,t6,t5,t4,t3,t2,t1,t0);key -= 4;
	DRound(t0,t7,t6,t5,t4,t3,t2,t1);key -= 4;
	DRound(t1,t0,t7,t6,t5,t4,t3,t2);key -= 4;
	DRound(t2,t1,t0,t7,t6,t5,t4,t3);key -= 4;
	DRound(t3,t2,t1,t0,t7,t6,t5,t4);key -= 4;
	DRound(t4,t3,t2,t1,t0,t7,t6,t5);key -= 4;
	DRound(t5,t4,t3,t2,t1,t0,t7,t6);key -= 4;
	DRound(t6,t5,t4,t3,t2,t1,t0,t7);key -= 4;
	DRound(t7,t6,t5,t4,t3,t2,t1,t0);key -= 4;
	DRound(t0,t7,t6,t5,t4,t3,t2,t1);key -= 4;
	DRound(t1,t0,t7,t6,t5,t4,t3,t2);key -= 4;
	DRound(t2,t1,t0,t7,t6,t5,t4,t3);key -= 4;
	DRound(t3,t2,t1,t0,t7,t6,t5,t4);key -= 4;
	DRound(t4,t3,t2,t1,t0,t7,t6,t5);key -= 4;
	DRound(t5,t4,t3,t2,t1,t0,t7,t6);key -= 4;
	DRound(t6,t5,t4,t3,t2,t1,t0,t7);key -= 4;
	DRound(t7,t6,t5,t4,t3,t2,t1,t0);key -= 4;
	DRound(t0,t7,t6,t5,t4,t3,t2,t1);key -= 4;
	DRound(t1,t0,t7,t6,t5,t4,t3,t2);key -= 4;
	DRound(t2,t1,t0,t7,t6,t5,t4,t3);key -= 4;
	DRound(t3,t2,t1,t0,t7,t6,t5,t4);key -= 4;
	DRound(t4,t3,t2,t1,t0,t7,t6,t5);key -= 4;
	DRound(t5,t4,t3,t2,t1,t0,t7,t6);key -= 4;
	DRound(t6,t5,t4,t3,t2,t1,t0,t7);

	DecFin_Transformation(t7, t1, t3, t5,key2[12],key2[13],key2[14],key2[15]);

	out[1] = t0; out[3] = t2; out[5] = t4; out[7] = t6;
}

void    HIGHT_KeySched(
            BYTE    *UserKey,     
            BYTE    *RoundKey)      
{
    int     i, j;

	//내가 만든 키

	for(i=0 ; i < BLOCK_SIZE_HIGHT ; i++)
	{
		for(j=0 ; j < BLOCK_SIZE_HIGHT ; j++)
			RoundKey[ 16*i + j ] = UserKey[(j-i)&7    ] + Delta[ 16*i + j ];

		for(j=0 ; j < BLOCK_SIZE_HIGHT ; j++)
			RoundKey[ 16*i + j + 8 ] = UserKey[((j-i)&7)+8] + Delta[ 16*i + j + 8 ];
	}

	//종료
}

DWORD* chartoint32_for_HIGHT_CBC( IN unsigned char *in, IN int inLen )
{
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
	DWORD *data = NULL;
#else
	DWORD data[INLENMAX];
#endif
	int len,i;

	if(inLen % 4)
		len = (inLen/4)+1;
	else
		len = (inLen/4);

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
	data = malloc(sizeof(unsigned int) * len);
#endif

	for(i=0;i<len;i++)
	{
		data[i] = ((unsigned int*)in)[i];
	}

	return data;  //mod.kks todo static or global value change.....
}

BYTE* int32tochar_for_HIGHT_CBC( IN DWORD *in, IN int inLen )
{
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
  unsigned char *data = malloc(sizeof(unsigned char) * inLen);
#else
  unsigned char data[INLENMAX];
#endif
	int i;

#ifndef BIG_ENDIAN_C
	for(i=0;i<inLen;i++)
	{
		data[i] = (unsigned char)(in[i/4] >> ((i%4)*8));
	}
#else
	for(i=0;i<inLen;i++)
	{
		data[i] = (unsigned char)(in[i/4] >> ((3-(i%4))*8));
	}
#endif

	return data; //mod.kks todo static or global value change.....
}

int HIGHT_CBC_init( OUT KISA_HIGHT_INFO *pInfo, IN KISA_ENC_DEC enc, IN const unsigned char *pUserKey, IN unsigned char *pbIV )
{
	unsigned char i, j;

	if( NULL == pInfo || 
		NULL == pUserKey ||
		NULL == pbIV )
	{
		if(enc == KISA_ENCRYPT)
		{
			return DAMO_CRYPT_ERR_HIGHT_ENC_NULL_POINTER;
		}
		else
		{
			return DAMO_CRYPT_ERR_HIGHT_DEC_NULL_POINTER;
		}
	}

	memset( pInfo, 0, sizeof(KISA_HIGHT_INFO) );
	pInfo->encrypt = enc;
	memcpy( (BYTE *)pInfo->ivec, (BYTE *)pbIV, BLOCK_SIZE_HIGHT );
	memcpy( pInfo->userKey, pUserKey, 16 );

	for(i=0 ; i < BLOCK_SIZE_HIGHT ; i++)
	{
		for(j=0 ; j < BLOCK_SIZE_HIGHT ; j++)
			pInfo->hight_key.key_data[ 16*i + j ] = pUserKey[(j-i)&7    ] + Delta[ 16*i + j ];

		for(j=0 ; j < BLOCK_SIZE_HIGHT ; j++)
			pInfo->hight_key.key_data[ 16*i + j + 8 ] = pUserKey[((j-i)&7)+8] + Delta[ 16*i + j + 8 ];
	}
	
	return DAMO_CRYPT_SUCCESS;
}

int HIGHT_CBC_Process( OUT KISA_HIGHT_INFO *pInfo, IN DWORD *in, IN int inLen, OUT DWORD *out, OUT int *outLen )
{
	int nCurrentCount = BLOCK_SIZE_HIGHT;
	DWORD *pdwXOR;

	if( NULL == pInfo ||
		NULL == in ||
		NULL == out ||
		0 > inLen )
	{
		if(pInfo->encrypt == KISA_ENCRYPT)
		{
			return DAMO_CRYPT_ERR_HIGHT_ENC_NULL_POINTER;
		}
		else
		{
			return DAMO_CRYPT_ERR_HIGHT_DEC_NULL_POINTER;
		}
	}

	pInfo->buffer_length = inLen - nCurrentCount;

	if( KISA_ENCRYPT == pInfo->encrypt )
	{
		pdwXOR = pInfo->ivec;

		while( nCurrentCount <= inLen )
		{
			BLOCK_XOR_HIGHT( out, in, pdwXOR );
			KISA_HIGHT_ECB_encrypt_forCBC( pInfo->hight_key.key_data, pInfo->userKey, (BYTE *)out, (BYTE *)out );
			pdwXOR = out;
			nCurrentCount += BLOCK_SIZE_HIGHT;
			in += BLOCK_SIZE_HIGHT_INT;
			out += BLOCK_SIZE_HIGHT_INT;
		}
		
		*outLen = nCurrentCount - BLOCK_SIZE_HIGHT;
		pInfo->buffer_length = inLen - *outLen;

		memcpy( pInfo->ivec, pdwXOR, BLOCK_SIZE_HIGHT );
		memcpy( pInfo->cbc_buffer, in, pInfo->buffer_length );

	}
	else
	{
		pdwXOR = pInfo->ivec;

		while( nCurrentCount <= inLen )
		{
			KISA_HIGHT_ECB_decrypt_forCBC( pInfo->hight_key.key_data, pInfo->userKey, (BYTE *)in, (BYTE *)out );
			
			BLOCK_XOR_HIGHT( out, out, pdwXOR );

			pdwXOR = in;

			nCurrentCount += BLOCK_SIZE_HIGHT;
			in += BLOCK_SIZE_HIGHT_INT;
			out += BLOCK_SIZE_HIGHT_INT;
		}
			
		*outLen = nCurrentCount - BLOCK_SIZE_HIGHT;
		memcpy( pInfo->ivec, pdwXOR, BLOCK_SIZE_HIGHT );
		memcpy( pInfo->cbc_last_block, out - BLOCK_SIZE_HIGHT_INT, BLOCK_SIZE_HIGHT );
	}

	return DAMO_CRYPT_SUCCESS;
}

int HIGHT_CBC_Close( OUT KISA_HIGHT_INFO *pInfo, IN DWORD *out, IN int out_offset, IN int *outLen, IN int pad )
{
   unsigned int nPaddngLeng;
   int i;
   BYTE *pOut;
   out += (out_offset/4);
   pOut = (BYTE *)(out);

   *outLen = 0;

   if( NULL == out )
   {
      return 0;   
   }

   if( KISA_ENCRYPT == pInfo->encrypt )
   {
	   nPaddngLeng = BLOCK_SIZE_HIGHT - pInfo->buffer_length;

	   for( i = pInfo->buffer_length; i<BLOCK_SIZE_HIGHT; i++ )
	   {
		   if(pad == DAMO_PKCS7_PADDING)
		   {
			   ((BYTE *)pInfo->cbc_buffer)[i] = (BYTE)nPaddngLeng;
		   }
		   else if(pad == DAMO_ZERO_PADDING)
		   {
			   ((BYTE *)pInfo->cbc_buffer)[i] = 0x00;
		   }
		   else
		   {
			   return DAMO_CRYPT_ERR_HIGHT_ENC_INVALID_PAD;
		   }
	   }       

	   BLOCK_XOR_HIGHT( pInfo->cbc_buffer, pInfo->cbc_buffer, pInfo->ivec );

	   KISA_HIGHT_ECB_encrypt_forCBC( pInfo->hight_key.key_data, pInfo->userKey, (BYTE *)pInfo->cbc_buffer, pOut );
	   out += BLOCK_SIZE_HIGHT_INT;
	   *outLen = BLOCK_SIZE_HIGHT;
   }
   else
   {
	   if(pad == DAMO_PKCS7_PADDING)
	   {
		   nPaddngLeng = ((BYTE*)pInfo->cbc_last_block)[BLOCK_SIZE_HIGHT-1];
		   if( nPaddngLeng > 0 && nPaddngLeng <= BLOCK_SIZE_HIGHT )
		   {
			   for (i = nPaddngLeng; i>0; i--)
			   {
				   *(pOut - i) = (BYTE)0x00;
			   }
			   *outLen = nPaddngLeng;
		   }
		   else 
			   return DAMO_CRYPT_ERR_HIGHT_DEC_GEOMJEUNG_PAD;
	   }
	   else if(pad == DAMO_ZERO_PADDING)
	   {
		   for ( i = 1 ; i <= BLOCK_SIZE_HIGHT ; i++)
		   {
			   if ( ((BYTE*)pInfo->cbc_last_block)[BLOCK_SIZE_HIGHT-i] != 0x00)
			   {
				   *outLen = i - 1;
				   return DAMO_CRYPT_SUCCESS;
			   }
		   }
		   *outLen = BLOCK_SIZE_HIGHT;
	   }
	   else
	   {
		   return DAMO_CRYPT_ERR_HIGHT_DEC_INVALID_PAD;
	   }
   }

   return DAMO_CRYPT_SUCCESS;
}

#if false //mod.kks 21.12.08  todo remove
int HIGHT_CBC_Encrypt( OUT unsigned char *out, size_t *outLen, IN const unsigned char *in, IN size_t inLen, IN const unsigned char *key, IN unsigned char *iv, size_t pad )
{
	KISA_HIGHT_INFO info;
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)    
	int outlen = 0; 
#endif    
	int nRetOutLeng = 0;
	int nPaddingLeng = 0;
	int nPlainTextPadding = 0;
	int ret = -1;
	unsigned int *data = NULL;
	unsigned char *cdata = NULL;

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
	unsigned int *outbuf = NULL;
	unsigned char *newpbszPlainText = NULL;
#else
	unsigned int outbuf[INLENMAX];
	unsigned char newpbszPlainText[INLENMAX];
#endif

  if(pad == DAMO_PKCS7_PADDING)
  {
     nPlainTextPadding = (BLOCK_SIZE_HIGHT - (inLen % BLOCK_SIZE_HIGHT));
  }
  else if(pad == DAMO_ZERO_PADDING)
  {
	  if (inLen % BLOCK_SIZE_HIGHT == 0)
	  {
		  nPlainTextPadding = 0;
	  }
	  else
	  {
		  nPlainTextPadding = (BLOCK_SIZE_HIGHT - (inLen % BLOCK_SIZE_HIGHT));
	  }	  
  }
  else
  {
     return DAMO_CRYPT_ERR_HIGHT_ENC_INVALID_PAD;
  }

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
  newpbszPlainText = malloc(sizeof(unsigned char) * (inLen + nPlainTextPadding));
#endif

	memcpy(newpbszPlainText, in, inLen);

	ret = HIGHT_CBC_init( &info, KISA_ENCRYPT, key, iv );
	if(ret != DAMO_CRYPT_SUCCESS)
    {
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
     free(newpbszPlainText);
#endif
     return ret;
    }

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
    outlen = ( (inLen + nPlainTextPadding)/8) *4;
	outbuf = malloc( sizeof(unsigned int) * outlen );
#endif

	data = (unsigned int *)chartoint32_for_HIGHT_CBC(newpbszPlainText, inLen);

	ret = HIGHT_CBC_Process( &info, (unsigned long*)data, inLen, (unsigned long*)outbuf, &nRetOutLeng );
	if(ret != DAMO_CRYPT_SUCCESS)
	{
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
     free(newpbszPlainText);
	 free(data);
	 free(outbuf);
#endif
	 return ret;
	}

	ret = HIGHT_CBC_Close( &info, (unsigned long*)outbuf, nRetOutLeng, &nPaddingLeng, pad );
	if(ret != DAMO_CRYPT_SUCCESS)
	{
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
     free(newpbszPlainText);
	 free(data);
	 free(outbuf);
#endif
	 return ret;
	}
	
	if(nPlainTextPadding == 0)
	{
		nPaddingLeng = 0;
	}

	cdata = int32tochar_for_HIGHT_CBC((unsigned long*)outbuf, nRetOutLeng + nPaddingLeng);
	memcpy(out, cdata, nRetOutLeng + nPaddingLeng);
    *outLen = nRetOutLeng+nPaddingLeng;

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
	free(newpbszPlainText);
	free(data);
	free(cdata);
	free(outbuf);
#endif

	return ret;
}

#endif

#if false //mod.kks 21.12.08
int HIGHT_CBC_Decrypt(OUT unsigned char *result, size_t *out_len, IN const unsigned char *pbszCipherText, IN size_t nCipherTextLen, IN const unsigned char *pbszUserKey, IN unsigned char *pbszIV, size_t pad)
{
	KISA_HIGHT_INFO info;
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)    //mod.kks todo check warnning ....
	int outlen = 0; 
#endif    
	int nRetOutLeng = 0;
	int nPaddingLeng = 0;
	int message_length = 0;
	int ret = -1;
	unsigned int *data = NULL;
    unsigned char *cdata = NULL;

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
  unsigned int *outbuf = NULL;
  BYTE *newpbszCipherText = NULL;
  BYTE *pbszPlainText = NULL;
#else
  unsigned int outbuf[INLENMAX];
  BYTE newpbszCipherText[INLENMAX];
  BYTE pbszPlainText[INLENMAX];
#endif
	
	if ((nCipherTextLen % BLOCK_SIZE_HIGHT) > 0)
	{
		return 0;
	}

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
  newpbszCipherText = malloc(sizeof(unsigned char) * (nCipherTextLen));
  pbszPlainText = malloc(sizeof(unsigned char) * (nCipherTextLen));
#endif

	memcpy(newpbszCipherText, pbszCipherText, nCipherTextLen);
	
	ret = HIGHT_CBC_init( &info, KISA_DECRYPT, pbszUserKey, pbszIV );
	if(ret != DAMO_CRYPT_SUCCESS)
	{
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
		free(newpbszCipherText);
		free(pbszPlainText);
#endif
		return ret;
	}
    
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
    outlen = ( (nCipherTextLen/8)) *4 ;
	outbuf = malloc(sizeof(unsigned int) * outlen);
#endif

	data = (unsigned int*)chartoint32_for_HIGHT_CBC(newpbszCipherText, nCipherTextLen);
	ret = HIGHT_CBC_Process( &info, (unsigned long*)data, nCipherTextLen, (unsigned long*)outbuf, &nRetOutLeng );
	if(ret != DAMO_CRYPT_SUCCESS)
	{
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
		free(newpbszCipherText);
		free(pbszPlainText);
		free(data);
		free(outbuf);
#endif
	    return ret;
	}

	ret = HIGHT_CBC_Close( &info, (unsigned long*)outbuf, nRetOutLeng, &nPaddingLeng, pad );
	if(ret != DAMO_CRYPT_SUCCESS)
	{
#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
		free(newpbszCipherText);
		free(pbszPlainText);
		free(data);
		free(outbuf);
#endif
	    return ret;
	}

	cdata = int32tochar_for_HIGHT_CBC( (unsigned long*)outbuf, nRetOutLeng - nPaddingLeng );
	memcpy( pbszPlainText, cdata, nRetOutLeng - nPaddingLeng );
	message_length = nRetOutLeng - nPaddingLeng;
	if (message_length < 0)
	{
		message_length = 0;
	}

	memcpy( result, pbszPlainText, message_length);
    *out_len = message_length;

#if defined(DUKPT_SERVER) || defined (DUKPT_CLIENT_ENCRYPT_ALLOC)
	free(newpbszCipherText);
	free(pbszPlainText);
	free(data);
	free(cdata);
	free(outbuf);
#endif

	return DAMO_CRYPT_SUCCESS;

}


int HIGHT_CFB_Encrypt(unsigned char *in, unsigned int inLen, const unsigned char *key, unsigned char *iv, unsigned int blockLen)
{
  int ret = -1;
  BYTE out[16];
  KISA_HIGHT_INFO info;  
  ret = HIGHT_CBC_init( &info, KISA_ENCRYPT, key, iv );
  if( ret != DAMO_CRYPT_SUCCESS)
  {
	  return ret;
  }

	while(1 <= inLen)
	{
		KISA_HIGHT_ECB_encrypt_forCFB( info.hight_key.key_data, info.userKey, iv, out);
		*in ^= iv[0];
		memmove(iv, iv+1, blockLen-1);
		iv[blockLen-1] = *in;
		inLen--;
		in++;
	}
	return inLen;
}

int HIGHT_CFB_Decrypt(unsigned char *in, unsigned int inLen, const unsigned char *key, unsigned char *iv, unsigned int blockLen)
{
  int ret = -1;
  BYTE c;
  BYTE out[16];
  KISA_HIGHT_INFO info;  
  ret = HIGHT_CBC_init( &info, KISA_ENCRYPT, key, iv );
  if(ret != DAMO_CRYPT_SUCCESS)
  {
	  return ret;
  }

  while (inLen >= 1) {
    c = *in;
    KISA_HIGHT_ECB_encrypt_forCFB( info.hight_key.key_data, info.userKey, iv, out);
    *in ^= iv[0];
    memmove(iv, iv+1, blockLen-1);
    iv[blockLen-1] = c;
    inLen--;
    in++;
  }
  return inLen; 
}

int DAMO_CRYPT_HIGHT_Decrypt_Core(OUT unsigned char *out, size_t *outLen, IN const unsigned char *in, IN size_t inLen, IN const unsigned char *key, int mode, IN unsigned char *iv, size_t pad)
{
  int ret = -1;

  switch(mode)
  {
    case CBC_MODE:
    {
      ret = HIGHT_CBC_Decrypt(out, outLen, in, inLen, key, iv, pad);
	}
	break;
	case CFB_MODE:
	{
	  memcpy(out, in, inLen);
      ret = HIGHT_CFB_Decrypt(out, inLen, key, iv, BLOCK_SIZE_HIGHT);
	  *outLen = inLen;
	}
    break;
  }	

  return ret;
}
#if false //mod.kks 21.12.09
int DAMO_CRYPT_HIGHT_Encrypt_Core( OUT unsigned char *out, size_t *outLen, IN const unsigned char *in, IN size_t inLen, IN const unsigned char *key, int mode, IN unsigned char *iv, size_t pad )
{

  int ret = -1;

  switch(mode)
  {
    case CBC_MODE:
    {
      ret = HIGHT_CBC_Encrypt(out, outLen, in, inLen, key, iv, pad);
	}
	break;
	case CFB_MODE:
	{
	  memcpy(out, in, inLen);
      ret = HIGHT_CFB_Encrypt(out, inLen, key, iv, BLOCK_SIZE_HIGHT);
	  *outLen = inLen;
	}
    break;
  }	

  return ret;
}
#endif
