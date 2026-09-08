#include <stdio.h>
#include <stdlib.h>
#include "aes.h"

void printdump( char *str, unsigned char *data, size_t len )
{
	int i;
	unsigned char *buf;

	buf = malloc(len*2+1);
	memset(buf, 0, len*2+1);

	for (i=0; i<len; i++)
		sprintf((char*)&buf[i*2],"%02x", data[i]);

	printf("%s : %s\n", str, buf);
	free(buf);
}

int aestest_main( int argc, char *argv[] )
{
	unsigned char key[16]={0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
		0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c};
	unsigned char iv[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
		0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char in[16]={0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
		0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a};
	unsigned char answer[16]={0x76,0x49,0xab,0xac,0x81,0x19,0xb2,0x46,
		0xce,0xe9,0x8e,0x9b,0x12,0xe9,0x19,0x7d};
	unsigned char answer2[16]={0x3b,0x3f,0xd9,0x2e,0xb7,0x2d,0xad,0x20,
		0x33,0x34,0x49,0xf8,0xe8,0x3c,0xfb,0x4a};
	unsigned char out[32]={0x00,};
	unsigned char out2[32]={0x00,};
	size_t out_len;
	size_t out2_len;


	printf("============================ AES-CBC ============================\n");
	printdump( "aes-cbc encrypt(plaintext)", in, 16 );
	DAMO_CRYPT_AES_EncryptEx(out, &out_len, in, 16, key, 16, AES_128, CBC_MODE, iv);
	printdump( "aes-cbc encrypt(ciphertext)", out, out_len );

	if(memcmp(out, answer, 16))
	{
		printf(" => Ciphertext is different from Answer of Test Vector\n");
	}
	else
	{
		printf(" => Ciphertext is same with Answer of Test Vector\n");
	}

	printdump( "aes-cbc decrypt(ciphertext)", out, out_len );
	DAMO_CRYPT_AES_DecryptEx(out2, &out2_len, out, out_len, key, 16, AES_128, CBC_MODE, iv);
	printdump( "aes-cbc decrypt(plaintext)", out2, out2_len );
	printf("\n");
	printf("=================================================================\r\n");

	printf("============================ AES-CFB ============================\r\n");
	printdump( "aes-cfb encrypt(plaintext)", in, 16 );
	DAMO_CRYPT_AES_EncryptEx(out, &out_len, in, 16, key, 16, AES_128, CFB_MODE, iv);
	printdump( "aes-cfb encrypt(ciphertext)", out, out_len );

	if(memcmp(out, answer2, 16))
	{
		printf(" => Ciphertext is different from Answer of Test Vector\n");
	}
	else
	{
		printf(" => Ciphertext is same with Answer of Test Vector\n");
	}

	printdump( "aes-cfb decrypt(ciphertext)", out, out_len );
	DAMO_CRYPT_AES_DecryptEx(out2, &out2_len, out, out_len, key, 16, AES_128, CFB_MODE, iv);
	printdump( "aes-cfb decrypt(plaintext)", out2, out2_len );
	printf("=================================================================\r\n");

	return 0;
}
