#include <stdio.h>
#include "dukpt_cli.h"
#include "aes.h"
#include "base64_penta.h"

//void printdump( char *str, unsigned char *data, size_t len )
//{
//	int i;
//	unsigned char *buf;
//
//	buf = malloc(len*2+1);
//	memset(buf, 0, len*2+1);
//
//	for (i=0; i<len; i++)
//		sprintf(&buf[i*2],"%02x", data[i]);
//
//	printf("%s : %s\n", str, buf);
//	free(buf);
//}

extern void printdump( char *str, unsigned char *data, size_t len );
extern void Print_Future_Key_Register();

int dukpttest_cli_main( int argc, char *argv[] )
{
	int i;
	unsigned char ipek[16]={0x6A,0xC2,0x92,0xFA,0xA1,0x31,0x5B,0x4D,
		0x85,0x8A,0xB3,0xA3,0xD7,0xD5,0x93,0x3A};
	unsigned char ksn[10]={0xFF,0xFF,0x98,0x76,0x54,
		0x32,0x10,0xE0,0x00,0x00};
	unsigned char *pin="1234";
	unsigned char *account_num="4012345678909";
	DukptPinEntry pEntry;
	int flags;

	//unsigned char in[16]={0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
	//	0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a};
	//unsigned char out[32]={0x00,};
	//unsigned char out2[32]={0x00,};
	//size_t out_len;
	//size_t out2_len;

	unsigned char enc_pin_block_tv[8] = {0x10,0xa0,0x1c,0x8d,0x02,0xc6,0x91,0x07};
	unsigned char req_mac_key_tv[16] = {0xc4,0x65,0x51,0xce,0xf9,0xfd,0xdb,0xb0,
		0xaa,0x9a,0xd8,0x34,0x13,0x0d,0xc4,0xc7};
	unsigned char res_mac_key_tv[16] = {0xc4,0x65,0x51,0xce,0x06,0xfd,0x24,0xb0,
		0xaa,0x9a,0xd8,0x34,0xec,0x0d,0x3b,0xc7};
	unsigned char req_enc_key_tv[16] = {0xf1,0xbe,0x73,0xb3,0x61,0x35,0xc5,0xc2,
		0x6c,0xf9,0x37,0xd5,0x0a,0xbb,0xe5,0xaf};
	unsigned char res_enc_key_tv[16] = {0xc1,0xc0,0xe2,0xd6,0x63,0xc5,0x0e,0xe9,
		0xc0,0x01,0xe5,0x6d,0x37,0x93,0xa4,0x79};

	DukptFutureKeyInfo fki;
	DukptFutureKeyInfo fkiNew;
  DukptFutureKeyInfoOld fkiOld;

	flags = DAMO_DUKPT_FLAG_USE_ALL_KEY;

	printf("============================ DUKPT TEST ============================\n\n");
	DAMO_DUKPT_Load_Initial_Key(ipek, sizeof(ipek), ksn, sizeof(ksn));
	printdump( "DAMO_DUKPT_Load_Initial_Key input(ipek)", ipek, 16 );
	printdump( "DAMO_DUKPT_Load_Initial_Key input(ksn)", ksn, 10 );
	Print_Future_Key_Register();
	printf("\n");

	DAMO_DUKPT_Request_Pin_Entry(pin, strlen((char const*)pin), account_num, strlen((char const*)account_num), flags, &pEntry);
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(pin)", pin, strlen((char const*)pin) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(account_num)", account_num, strlen((char const*)account_num) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(ksn)", pEntry.ksn, 10 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(enc_pin_block)", pEntry.enc_pin_block, 8 );
	if(memcmp(pEntry.enc_pin_block, enc_pin_block_tv, 8))
	{
		printf(" => enc_pin_block is different from Answer of Test Vector\n");
	}
	else
	{
		printf(" => enc_pin_block is same with Answer of Test Vector\n");
	}
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_mac_key)", pEntry.req_mac_key, 16 );
	if(memcmp(pEntry.req_mac_key, req_mac_key_tv, 16))
	{
		printf(" => req_mac_key is different from Answer of Test Vector\n");
	}
	else
	{
		printf(" => req_mac_key is same with Answer of Test Vector\n");
	}
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_mac_key)", pEntry.res_mac_key, 16 );
	if(memcmp(pEntry.res_mac_key, res_mac_key_tv, 16))
	{
		printf(" => res_mac_key is different from Answer of Test Vector\n");
	}
	else
	{
		printf(" => res_mac_key is same with Answer of Test Vector\n");
	}
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_enc_key)", pEntry.req_enc_key, 16 );
	if(memcmp(pEntry.req_enc_key, req_enc_key_tv, 16))
	{
		printf(" => req_enc_key is different from Answer of Test Vector\n");
	}
	else
	{
		printf(" => req_enc_key is same with Answer of Test Vector\n");
	}
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_enc_key)", pEntry.res_enc_key, 16 );
	if(memcmp(pEntry.res_enc_key, res_enc_key_tv, 16))
	{
		printf(" => res_enc_key is different from Answer of Test Vector\n");
	}
	else
	{
		printf(" => res_enc_key is same with Answer of Test Vector\n");
	}

	if(DAMO_DUKPT_Export_Future_Key_Info(&fki)<0)
	{
  	    printf("Error!!! Exporting Future Key Info!!!\n\n");
		return 1;
	}
	printf("\n");
	printf("Exporting Future Key Info!!!\n");
	printf("Base64 Encoded KSN : %s\n", fki.key_serial_number);
	for(i=0; i<21; i++)
//		printf("Base64 Encoded %d-Future Key : %s\n", i, fki.future_key_register[i]);
	printf("\n");
#if 0
	Print_Future_Key_Register();
	printf("\n");
#endif

  /* Read CBC Future Keys Import Test */
  memcpy(&fkiNew, &fki, sizeof(DukptFutureKeyInfo));

	if(DAMO_DUKPT_Import_Future_Key_Info(&fkiNew)<0)
	{
  	printf("Error!!! Importing CBC Future Key Info!!!\n\n");
		return 1;
	}
	printf("\n");
	printf("Importing CBC Future Key Info!!!\n\n");

	DAMO_DUKPT_Request_Pin_Entry(pin, strlen((char const*)pin), account_num, strlen((char const*)account_num), flags, &pEntry);
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(pin)", pin, strlen((char const*)pin) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(account_num)", account_num, strlen((char const*)account_num) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(ksn)", pEntry.ksn, 10 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(enc_pin_block)", pEntry.enc_pin_block, 8 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_mac_key)", pEntry.req_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_mac_key)", pEntry.res_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_enc_key)", pEntry.req_enc_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_enc_key)", pEntry.res_enc_key, 16 );
#if 0
	Print_Future_Key_Register();
	printf("\n");
#endif

	DAMO_DUKPT_Request_Pin_Entry(pin, strlen((char const*)pin), account_num, strlen((char const*)account_num), flags, &pEntry);
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(pin)", pin, strlen((char const*)pin) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(account_num)", account_num, strlen((char const*)account_num) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(ksn)", pEntry.ksn, 10 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(enc_pin_block)", pEntry.enc_pin_block, 8 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_mac_key)", pEntry.req_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_mac_key)", pEntry.res_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_enc_key)", pEntry.req_enc_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_enc_key)", pEntry.res_enc_key, 16 );
#if 0
	Print_Future_Key_Register();
	printf("\n");
#endif

printf("\nCFB Old Future Key Import Test\n");
  /* Set CFB Old Key */
  strcpy((char*)fkiOld.key_serial_number,       "//+YdlQyEOAAAw==");
  strcpy((char*)fkiOld.future_key_register[0],  "RRcE9vfx+p4XJMopPN48uKU=");
  strcpy((char*)fkiOld.future_key_register[1],  "aOPEEA/sOVHS+gpamGAwHgY=");
  strcpy((char*)fkiOld.future_key_register[2],  "cAO0fvkUOE5MK8P69znnc/A=");
  strcpy((char*)fkiOld.future_key_register[3],  "Mae8x4KMiKZwQ4bYVOhh9lU=");
  strcpy((char*)fkiOld.future_key_register[4],  "wMBQJtQj9oqny+98P7kbuZw=");
  strcpy((char*)fkiOld.future_key_register[5],  "9TiEtIsFEj6zEfZnrdH7+y0=");
  strcpy((char*)fkiOld.future_key_register[6],  "iIR3IxUvvZV72bZBECFqCTM=");
  strcpy((char*)fkiOld.future_key_register[7],  "fasSnPd0ZTkB7/JLErCwtYI=");
  strcpy((char*)fkiOld.future_key_register[8],  "7hGGxEkTOodK9G5FsJYNwpE=");
  strcpy((char*)fkiOld.future_key_register[9],  "kRDsKNDk9wgAhqM/XEyLCW0=");
  strcpy((char*)fkiOld.future_key_register[10], "7D0VPbqlRStBkP+suhvIADs=");
  strcpy((char*)fkiOld.future_key_register[11], "WbultfSf7q7TSXp4WKw/D1Y=");
  strcpy((char*)fkiOld.future_key_register[12], "SbawbYs3hvhC7YuvNuyfoyg=");
  strcpy((char*)fkiOld.future_key_register[13], "jguSseKJusbkSiVp74HtHLk=");
  strcpy((char*)fkiOld.future_key_register[14], "ZelXcTkibOYTZKb3yGzDuAs=");
  strcpy((char*)fkiOld.future_key_register[15], "RnJvZyFLiR6s7wqJSQeHMUE=");
  strcpy((char*)fkiOld.future_key_register[16], "tgPR5ktyqRTL9cnia/uxWrk=");
  strcpy((char*)fkiOld.future_key_register[17], "yKwxf9YwXzX1jPQm1UisjbI=");
  strcpy((char*)fkiOld.future_key_register[18], "yMZTR3wfg7F0z+Y4KyBAnAo=");
  strcpy((char*)fkiOld.future_key_register[19], "71pcLZLPPdRf45UGOKPuDWI=");
  strcpy((char*)fkiOld.future_key_register[20], "4qmFb7gFazEYhPgBlchD910=");

  /* Read CFB Old Key  */
  memset(&fki, 0, sizeof(DukptFutureKeyInfo));
  memcpy(&fki, &fkiOld, sizeof(DukptFutureKeyInfoOld));

  /* Import CFB Old Key */
	if(DAMO_DUKPT_Import_Future_Key_Info(&fki)<0)
	{
  	printf("Error!!! Importing CFB Future Key Info!!!\n\n");
		return 1;
	}
	printf("\n");
	printf("Importing CFB Future Key Info!!!\n\n");

	DAMO_DUKPT_Request_Pin_Entry(pin, strlen((char const*)pin), account_num, strlen((char const*)account_num), flags, &pEntry);
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(pin)", pin, strlen((char const*)pin) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(account_num)", account_num, strlen((char const*)account_num) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(ksn)", pEntry.ksn, 10 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(enc_pin_block)", pEntry.enc_pin_block, 8 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_mac_key)", pEntry.req_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_mac_key)", pEntry.res_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_enc_key)", pEntry.req_enc_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_enc_key)", pEntry.res_enc_key, 16 );
#if 0
	Print_Future_Key_Register();
	printf("\n");
#endif

	DAMO_DUKPT_Request_Pin_Entry(pin, strlen((char const*)pin), account_num, strlen((char const*)account_num), flags, &pEntry);
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(pin)", pin, strlen((char const*)pin) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry input(account_num)", account_num, strlen((char const*)account_num) );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(ksn)", pEntry.ksn, 10 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(enc_pin_block)", pEntry.enc_pin_block, 8 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_mac_key)", pEntry.req_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_mac_key)", pEntry.res_mac_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(req_enc_key)", pEntry.req_enc_key, 16 );
	printdump( "DAMO_DUKPT_Request_Pin_Entry ouput(res_enc_key)", pEntry.res_enc_key, 16 );
#if 0
	Print_Future_Key_Register();
	printf("\n");
#endif

	printf("====================================================================\r\n");
	
	return 0;
}
