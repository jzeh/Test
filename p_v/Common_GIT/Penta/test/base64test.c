#include <stdio.h>
#include <stdlib.h>
#include "GIT_base64.h"

void printdump( char *str, unsigned char *data, size_t len )
{
	int i;
	unsigned char *buf;

	buf = malloc(len*2+1);
	memset(buf, 0, len*2+1);

	for (i=0; i<len; i++)
		sprintf(&buf[i*2],"%02x", data[i]);

	printf("%s : %s\n", str, buf);
	free(buf);
}

int main( int argc, char *argv[] )
{
	unsigned char ksn[10]={0xFF,0xFF,0x98,0x76,0x54,
		0x32,0x10,0xE0,0x00,0x00};
	unsigned char enc_ksn[17]={0x00,};
	size_t enc_ksn_len = 17;
	unsigned char dec_ksn[10]={0x00,};
	size_t dec_ksn_len = 10;
	unsigned char key[17]={0x6A,0xC2,0x92,0xFA,0xA1,0x31,0x5B,0x4D,
		0x85,0x8A,0xB3,0xA3,0xD7,0xD5,0x93,0x3A,0x00};
	unsigned char enc_key[25]={0x00,};
	size_t enc_key_len = 25;
	unsigned char dec_key[17]={0x00,};
	size_t dec_key_len = 17;

	printf("============================ BASE64 ============================\r\n");
	printdump( "KSN", ksn, 10 );
	DAMO_CRYPT_Base64_Encode( enc_ksn, &enc_ksn_len, ksn, 10 );
	printf("Base64 Encoded KSN(%d) : %s\n", enc_ksn_len, enc_ksn);
	DAMO_CRYPT_Base64_Decode( dec_ksn, &dec_ksn_len, enc_ksn, enc_ksn_len );
	printdump( "Base64 Decoded KSN", dec_ksn, dec_ksn_len );
	printdump( "KEY", key, 17 );
	DAMO_CRYPT_Base64_Encode( enc_key, &enc_key_len, key, 17 );
	printf("Base64 Encoded KSN(%d) : %s\n", enc_key_len, enc_key);
	DAMO_CRYPT_Base64_Decode( dec_key, &dec_key_len, enc_key, enc_key_len );
	printdump( "Base64 Decoded KSN", dec_key, dec_key_len );
	printf("================================================================\r\n");

	return 0;
}
