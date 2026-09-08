#include "FreeRTOS.h"
#include "string.h"
#include "git_hsm.h"
#include "stdlib.h"
#include "firmware.h"
#include "common.h"
#include "usart.h"
#include "git_fsutil.h"

uint8_t apdu[500] = {0x00,};
uint8_t resp[500] = {0x00,};
uint8_t sw[2];
uint8_t counter[16] = {0x00,};

uint8_t overflow = 0x00;
uint8_t block_num = 0x00;
uint16_t temp = 0x0000;
uint16_t block_len = 0x00;
uint16_t remain_len = 0x00;

uint8_t DK_ENC[24] = {0x00, };
uint8_t DK_MAC[24] = {0x00, };
uint8_t DK_DEK[24] = {0x00, };
	
uint8_t ret;

extern uint8_t g_ucAES256_Key[32];

extern uint8_t *pUpdate_APDU; //Edit by Q
extern SHSMUpdateAck *pHSMAck;
extern void HSM_Update_Ack( bool state, int progress );
// Applet Update APDU
uint8_t update_APDU[27][260]; // Please add the GITSaveCerti_20220610_V10.txt file.
		
#define HSM_UART_PORT										huart6
#define	ISO7816_BAUDRATE									HSM_OSC_CLK / 372
#define HSM_OSC_CLK											8000000					// 3.686Mhz
		
	
void TDES_CBC(uint8_t *data, uint8_t *key, uint8_t *Edata, uint8_t dataLen)
{	
    uint8_t IV[8] = {0x00, };//
	getTDESEncoding_CBC(data, dataLen, key, IV, sizeof(IV), Edata, NULL);
}
void TDES_ECB(uint8_t *data, uint8_t *key, uint8_t *Edata, uint8_t dataLen)
{	
  getTDESEncoding_ECB(data, dataLen, key, Edata, NULL); 
}
void DES_ENC(uint8_t *data, uint8_t *key, uint8_t *Edata, uint8_t dataLen)
{	
  getDESEncoding_ECB(data, dataLen, key, Edata, NULL);
}
void DES_DEC(uint8_t *data, uint8_t *key, uint8_t *Edata, uint8_t dataLen)
{	
  getDESDecoding_ECB(data, dataLen, key, Edata, NULL);
}



void send_APDU(uint8_t *command, uint16_t cLen, uint8_t *resp, uint8_t *sw)
{	
  uint32_t len;
		
	//osDelay(100);	
	se_tranreceive( command, cLen, resp, &len );
	
	if (len >= 2)	// success
    {		
        memcpy(sw, &resp[len-2], 2);
	}
	else
		sw[0] = 0;
	
	if (sw[0] == 0x61)	// 61xx	
    {		
        command[0] = 0x00;
		command[1] = 0xC0;
		command[2] = 0x00;
		command[3] = 0x00;
		command[4] = sw[1];
		
		se_tranreceive(command, 5, resp, &len);
		
		if (len >= 2)	// success
        {			
            memcpy(sw, &resp[len-2], 2);
		}
		else
			sw[0] = 0;
	}
}


int StoreKeyHSM(uint8_t *EKey, int KeyLen, uint8_t index)
{	
    if (KeyLen != SIZE_DATA_RSA1024) return HSM_INVALID_KEYLENGTH;
	
	// Initialize crypto
	apdu[0] = 0x90;
	apdu[1] = 0x2A;
	apdu[2] = 0x88;
	apdu[3] = index;
	apdu[4] = 0x00;
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
    {		
        // Perform crypto
		// 902E0000xx
		apdu[0] = 0x90;
		apdu[1] = 0x2E;
		apdu[2] = 0x00;
		apdu[3] = 0x00;
		apdu[4] = KeyLen;
		memcpy(&apdu[5], EKey, KeyLen);
		send_APDU(apdu, KeyLen+5, resp, sw);
		
		if (sw[0] == 0x90 && sw[1] == 0x00) return 0;
		else return HSM_NOT_RESPONSE;
	}
	else if (sw[0] == 0x69 && sw[1] == 0x85) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}

int StoreAES128HSM(uint8_t *EKey, int KeyLen, uint8_t index, uint8_t *Checksum)
{	
    if (KeyLen != SIZE_DATA_RSA1024) return HSM_INVALID_KEYLENGTH;
	
	// StoreAES128
	apdu[0] = 0x90;
	apdu[1] = 0x28;
	apdu[2] = 0x00;
	apdu[3] = index; // 0x00, 0x01, 0x02, 0x03, 0x04
	apdu[4] = 0x80;
	memcpy(&apdu[5], EKey, 0x80);
	
	send_APDU(apdu, 0x85, resp, sw);
	
	Checksum[0] = resp[0];
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else if (sw[0] == 0x69 && sw[1] == 0x82) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}

int StoreAES128KeyHSMforECUcode(uint8_t *EKey, int KeyLen, uint8_t *Checksum)
{		
    if (KeyLen != SIZE_DATA_RSA1024) return HSM_INVALID_KEYLENGTH;
	
	// StoreAES128
	apdu[0] = 0x90;
	apdu[1] = 0x28;
	apdu[2] = 0x00;
	apdu[3] = 0x05;
	apdu[4] = 0x80;
	memcpy(&apdu[5], EKey, 0x80);
	
	send_APDU(apdu, 0x85, resp, sw);
	
	Checksum[0] = resp[0];
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else if (sw[0] == 0x69 && sw[1] == 0x82) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}

int ReadAES128KeyChecksumHSM(uint8_t index, uint8_t *Checksum)
{			
	// Read Checksum
	apdu[0] = 0x90;
	apdu[1] = 0x28;
	apdu[2] = 0x02;
	apdu[3] = index;	// 0x00 ~ 0x05
	apdu[4] = 0x01;
		
	send_APDU(apdu, 0x05, resp, sw);
	
	Checksum[0] = resp[0];
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else if (sw[0] == 0x69 && sw[1] == 0x82) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}

int DecryptAES128HSM(uint8_t *EData, uint8_t *PData, uint8_t mode, uint8_t Keyindex, uint16_t DataLen)
{	
    uint8_t i, j =0;
	
	// Decrypt Message
	// CTR mode --> EData = IV || Encrypted Data , DataLen = Encrypted Data size + 16 
	apdu[0] = 0x90;
	apdu[1] = 0x3E;
	apdu[2] = mode;			// 00:ECB, 01:CBC, 02:CTR
	apdu[3] = Keyindex; // 0x00, 0x01, 0x02, 0x03, 0x04
	apdu[4] = DataLen;
	memcpy(&apdu[5], EData, DataLen);
	
	
	if(apdu[2] == 0x02)
    {	
        // CTR mode 
		// block_num = (uint8_t)(DataLen - 16) / 16;
		block_num = (uint16_t)(DataLen - 16) / 0x60;
		block_len = DataLen;
		counter[15] = 0;
		// block_len = 16;
		block_len = 0x60;
		// remain_len = DataLen-16;
		remain_len = DataLen-16-0x60;
		
		for(i = 0 ; i <= block_num ; i++)
        {			
			overflow = 0;
			
			for( j = 0 ; j <= 15 ; j++){ // IV++, IV = apud[5~20]
				temp = (uint16_t)(apdu[5+15-j] & 0x00FF);
				temp = temp + (uint16_t)(counter[15-j] & 0x00FF) + overflow;
				if(temp > 0x00FF)
                {					
                    overflow = 1;
				}
				else
                {					
                    overflow = 0;
				}
				apdu[5+15-j] = (uint8_t)temp;
			}
			// counter[15] = 1;
			counter[15] = 6;
			
			// memcpy(&apdu[21], &EData[16*(i+1)], block_len);
			memcpy(&apdu[21], &EData[16 + (0x60*i)], block_len);
			apdu[0] = 0x90;
			apdu[1] = 0x3E;
			apdu[2] = mode;			// 00:ECB, 01:CBC, 02:CTR
			apdu[3] = Keyindex; // 0x00, 0x01, 0x02, 0x03, 0x04
			apdu[4] = block_len + 16;
			send_APDU(apdu, block_len + 16 + 5, resp, sw);
			if (sw[0] == 0x90 && sw[1] == 0x00)
            {				
                // memcpy(&PData[16*i], resp, block_len);
				memcpy(&PData[0x60*i], resp, block_len);
				// remain_len -= 16;
				// if(remain_len >= 16)
				if(remain_len >= 0x60)
                {                    
                    remain_len -= 0x60;
					// block_len = 16;
					block_len = 0x60;
					// apdu[4] = 32;
					apdu[4] = 0x60 + 0x10;
				}
				else
                {					
                    block_len = remain_len;					
					apdu[4] = 16 + remain_len;
				}
			}
			else return HSM_NOT_RESPONSE;
		}
		return HSM_SUCCESS;
	}
	else
    { 
        // ECB,CBC
		send_APDU(apdu, DataLen + 5, resp, sw);
	
		
		if (sw[0] == 0x90 && sw[1] == 0x00)
        {			
            if(mode == 0x02) DataLen -= 16; // CTR mode
						
			memcpy(PData, resp, DataLen);
			return HSM_SUCCESS;
		}
		else if (sw[0] == 0x69 && sw[1] == 0x82) return HSM_NEED_MUTUAL_AUTH;
		else return HSM_NOT_RESPONSE;
	}
}


int StoreHMACSHA256KeyHSM(uint8_t *EKey, int KeyLen, uint8_t index)
{	
    if (KeyLen != SIZE_DATA_RSA1024) return HSM_INVALID_KEYLENGTH;
	
	// Store HMAC SHA256 Key
	apdu[0] = 0x90;
	apdu[1] = 0x28;
	apdu[2] = 0x01;
	apdu[3] = index; // 0x00, 0x01, 0x02, 0x03, 0x04
	apdu[4] = 0x80;
	memcpy(&apdu[5], EKey, 0x80);
	
	send_APDU(apdu, 0x85, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else if (sw[0] == 0x69 && sw[1] == 0x82) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}

int HMACSHA256SignHSM(uint8_t *Data, uint8_t *EData, uint8_t Keyindex, uint16_t DataLen)
{	
    uint8_t i =0;
	uint8_t mode = 0; // 0x00 : one time, 0x01 : first mode for several time, 0x02 : next  mode, 0x03 : Last mode
	
	block_len = 0x70;
	remain_len = DataLen;
	// Setting mode
	if(DataLen <= block_len)
    {		
        mode = 0x00;
		
		// Sgin HMAC SHA256
		apdu[0] = 0x90;
		apdu[1] = 0x36;
		apdu[2] = mode;			
		apdu[3] = Keyindex; // 0x00, 0x01, 0x02, 0x03, 0x04
		apdu[4] = DataLen;
		memcpy(&apdu[5], Data, DataLen);
		send_APDU(apdu, DataLen + 5, resp, sw);
	
	} 
	else
    {		
        mode = 0x01; // first		
		block_num = (uint8_t)(DataLen / block_len);
		
		// Sgin HMAC SHA256
		apdu[0] = 0x90;
		apdu[1] = 0x36;
		apdu[2] = mode;			// First
		apdu[3] = Keyindex; // 0x00, 0x01, 0x02, 0x03, 0x04
		apdu[4] = block_len;
		memcpy(&apdu[5], Data, block_len);
		send_APDU(apdu, block_len + 5, resp, sw);
		remain_len = DataLen - block_len;
		
		for(i = 1 ; i < block_num ; i++)
        {			
            mode = 0x02; // Next
			apdu[0] = 0x90;
			apdu[1] = 0x36;
			apdu[2] = mode;			// First
			apdu[3] = Keyindex; // 0x00, 0x01, 0x02, 0x03, 0x04
			apdu[4] = block_len;
			memcpy(&apdu[5], &Data[block_len * i], block_len);
			send_APDU(apdu, block_len + 5, resp, sw);
			
			remain_len -= block_len;
			
		}
		mode = 0x03;	// Last
		apdu[0] = 0x90;
		apdu[1] = 0x36;
		apdu[2] = mode;			// First
		apdu[3] = Keyindex; // 0x00, 0x01, 0x02, 0x03, 0x04
		apdu[4] = remain_len;
		memcpy(&apdu[5], &Data[block_len * i], remain_len);
		send_APDU(apdu, remain_len + 5, resp, sw);
	
	
	}
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
    {		
        memcpy(EData, resp, 0x20);
		return HSM_SUCCESS;
	}
	else if(sw[0] == 0x69 && sw[1] == 0x82) return HSM_NEED_MUTUAL_AUTH;
	else 
		return HSM_NOT_RESPONSE;
	}


int MutualAuthHSM()
{	
    uint8_t i =0;
	//uint8_t DK_ENC[16] = {0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31};
	//uint8_t DK_MAC[16] = {0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31};
	//uint8_t DK_DEK[16] = {0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31};
    //uint8_t DK_ENC[24] = {0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31, 0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63};
	//uint8_t DK_MAC[24] = {0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31, 0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63};
	//uint8_t DK_DEK[24] = {0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31, 0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63};
    //uint8_t DK_ENC[24] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
	//uint8_t DK_MAC[24] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
	//uint8_t DK_DEK[24] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
	uint8_t HRN[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	uint8_t KEY_DERIVATION_DATA[10] = {0x00,};
	uint8_t KEY_VERSION = 0x00;
	uint8_t SCP = 0x00;
	uint8_t SEQ_COUNT[2] = {0x00,};
	uint8_t CRN[6] = {0x00,};
	uint8_t CARD_CRYPTO[8] = {0x00,};
	uint8_t HOST_CRYPTO[8] = {0x00,};
	uint8_t temp_Data[24] = {0x00,};
	uint8_t encrypted_Data[24] = {0x00,};
	uint8_t ENC_CONST[2] = {0x01, 0x82};
	uint8_t MAC_CONST[2] = {0x01, 0x01};
	uint8_t DEK_CONST[2] = {0x01, 0x81};
	uint8_t SK_ENC[24] = {0x00,};
	uint8_t SK_MAC[24] = {0x00,};
	uint8_t SK_DEK[24] = {0x00,};
	uint8_t MAC[8] = {0x00,};
		
	se_open();
		
	// Select ISD A000000151000000
	apdu[0] = 0x00;
	apdu[1] = 0xA4;
	apdu[2] = 0x04;
	apdu[3] = 0x00;
	apdu[4] = 0x08;

	apdu[5] = 0xA0;
	apdu[6] = 0x00;
	apdu[7] = 0x00;
	apdu[8] = 0x01;
	apdu[9] = 0x51;
	apdu[10] = 0x00;
	apdu[11] = 0x00;
	apdu[12] = 0x00;

	send_APDU(apdu, 13, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
    {		
        // Initialze Update
		apdu[0] = 0x80;
		apdu[1] = 0x50;
		apdu[2] = 0x00;
		apdu[3] = 0x00;
		apdu[4] = 0x08;
		memcpy(&apdu[5], HRN, 8);
		
		send_APDU(apdu, 13, resp, sw);
		if (sw[0] == 0x90 && sw[1] == 0x00)
        {			
            memcpy(KEY_DERIVATION_DATA, resp, 10);
			memcpy(&KEY_VERSION, resp + 10, 1);
			memcpy(&SCP, resp + 11, 1);
			memcpy(SEQ_COUNT, resp + 12, 2);
			memcpy(CRN, resp + 14, 6);
			memcpy(CARD_CRYPTO, resp + 20, 8);
			
			// Session Key
			memcpy(&temp_Data[0], ENC_CONST, 2);
			memcpy(&temp_Data[2], SEQ_COUNT, 2);
			memset(&temp_Data[4], 0x00, 12);
			TDES_CBC(temp_Data, DK_ENC, SK_ENC, 16);
            memcpy(&SK_ENC[16], &SK_ENC[0], 8);
			
			memcpy(&temp_Data[0], MAC_CONST, 2);
			memcpy(&temp_Data[2], SEQ_COUNT, 2);
			memset(&temp_Data[4], 0x00, 12);
			TDES_CBC(temp_Data, DK_MAC, SK_MAC, 16);
            memcpy(&SK_MAC[16], &SK_MAC[0], 8);
			
			memcpy(&temp_Data[0], DEK_CONST, 2);
			memcpy(&temp_Data[2], SEQ_COUNT, 2);
			memset(&temp_Data[4], 0x00, 12);
			TDES_CBC(temp_Data, DK_DEK, SK_DEK, 16);
            memcpy(&SK_DEK[16], &SK_DEK[0], 8);
			
			// Verify Card Cryptogram
			memcpy(&temp_Data[0], HRN, 8);
			memcpy(&temp_Data[8], SEQ_COUNT, 2);
			memcpy(&temp_Data[10], CRN, 6);
			temp_Data[16] = 0x80;
			memset(&temp_Data[17], 0x00, 7);
			TDES_CBC(temp_Data, SK_ENC, encrypted_Data, 24); // TDES_MAC
			for(i = 0 ; i < 4 ; i++)
            {				
                if(CARD_CRYPTO[i] != encrypted_Data[16 + i]) return HSM_NOT_RESPONSE;
			}
			
			// Generate Host Cryptogram
			memcpy(&temp_Data[0], SEQ_COUNT, 2);
			memcpy(&temp_Data[2], CRN, 6);
			memcpy(&temp_Data[8], HRN, 8);
			temp_Data[16] = 0x80;
			memset(&temp_Data[17], 0x00, 7);
			TDES_CBC(temp_Data, SK_ENC, encrypted_Data, 24); // TDES_MAC
			memcpy(HOST_CRYPTO , &encrypted_Data[16], 8);
			
			// Generate MAC
			temp_Data[0] = 0x84;
			temp_Data[1] = 0x82;
			temp_Data[2] = 0x00;
			temp_Data[3] = 0x00;
			temp_Data[4] = 0x10;
			memcpy(&temp_Data[5], HOST_CRYPTO, 8);
			temp_Data[13] = 0x80;
			temp_Data[14] = 0x00;
			temp_Data[15] = 0x00;
			DES_ENC(temp_Data, SK_MAC, encrypted_Data, 8); // DES_TDES_MAC
			for(i = 0 ; i < 8 ; i++)
            {				
                temp_Data[8+i] ^= encrypted_Data[i];
			}			
			DES_ENC(&temp_Data[8], SK_MAC, encrypted_Data, 8); // DES_TDES_MAC
			memcpy(temp_Data, encrypted_Data, 8);
			DES_DEC(temp_Data, &SK_MAC[8], encrypted_Data, 8); // DES_TDES_MAC
			memcpy(temp_Data, encrypted_Data, 8);
			DES_ENC(temp_Data, SK_MAC, MAC, 8); // DES_TDES_MAC
			
			// External Authentication
			apdu[0] = 0x84;
			apdu[1] = 0x82;
			apdu[2] = 0x00;
			apdu[3] = 0x00;
			apdu[4] = 0x10;
			
			memcpy(&apdu[5], HOST_CRYPTO, 8);
			memcpy(&apdu[13], MAC, 8);
			send_APDU(apdu, 21, resp, sw);
			if (sw[0] == 0x90 && sw[1] == 0x00)
            {				
                return HSM_SUCCESS;
			}
			else
            {				
                return HSM_NOT_RESPONSE;
			}
			
		}			
		else
        {			
            return HSM_NOT_RESPONSE;
		}
				
	}
	else
    {		
        return HSM_NOT_RESPONSE;
	}
	
}

int ReadVersionHSM(uint8_t *NumVersion)
{	
	// Get Data version number
	apdu[0] = 0x00;
	apdu[1] = 0xCA;
	apdu[2] = 0x01;
	apdu[3] = 0x02;
	apdu[4] = 0x02;
		
	send_APDU(apdu, 0x05, resp, sw);
	
	memcpy(NumVersion, resp, 2);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
}

int UpdateAppletHSM(uint8_t APDU_blocknum)
{	
    int i = 0;
    U8 Retry = 0;
	// 80E60C002206A0004749541007A000474954100107A0004749541001011007C905044607025A00
	uint8_t APDU_Install[39] = {0x80, 0xE6, 0x0C, 0x00, 0x22, 0x06, 0xA0, 0x00, 0x47, 0x49, 0x54, 0x10, 0x07, 0xA0, 0x00, 0x47, 0x49, 0x54, 0x10, 0x01, 0x07, 0xA0, 0x00, 0x47, 0x49, 0x54, 0x10, 0x01, 0x01, 0x10, 0x07, 0xC9, 0x05, 0x04, 0x46, 0x07, 0x02, 0x5A, 0x00};

	// Send these commands after Reset 
	// 20240328 
	MutualAuthHSM();
	
    for(i = 0 ; i < APDU_blocknum ; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 260))
		{			
            Retry++;
			if(Retry > 3) return HSM_UNKNOWN_ERROR;
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		GLogN(".");
        
        pHSMAck->mProgress = 94 + ((i*5)/APDU_blocknum);
        
        if(i%5 == 0)
        {
            HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
        }
        
        osDelay(1);
	}

	// Install for Install 
	memcpy(apdu, APDU_Install, 39);
	send_APDU(apdu, 39, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
			
}

int DeleteAppletHSM(void)
{	
  
	// 80E40080084F06A00047495410
	uint8_t APDU_Delete[13] = {0x80, 0xE4, 0x00, 0x80, 0x08, 0x4F, 0x06, 0xA0, 0x00, 0x47, 0x49, 0x54, 0x10};

	// Send these commands after Reset 
	// 20240610 
	MutualAuthHSM();
	
  // Delete  
	memcpy(apdu, APDU_Delete, 13);
	send_APDU(apdu, 13, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
			
}

extern uint8_t g_HSM_update_flag;
extern uint8_t g_HSM_ATR_Flag;
int UpdateOSHSM(uint32_t APDU_blocknum)
{	
    int i = 0;
	//uint8_t APDU_BtL_01[21] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0x62, 0x48, 0xF6, 0xC5, 0x61, 0x57, 0x35, 0x28, 0xD4, 0x80, 0x44, 0xD7, 0x7B, 0xF8, 0xAE, 0xC7};
	//uint8_t APDU_BtL_02[5] = {0x80, 0xBB, 0x00, 0x00, 0x00};
	//uint8_t APDU_BtL_03[21] = {0x80, 0x88, 0x00, 0x00, 0x10, 0xFF, 0x65, 0xC1, 0x22, 0x70, 0x43, 0x52, 0x61, 0xC4, 0x6E, 0xEE, 0xB5, 0xDC, 0xD8, 0xB6, 0x5A};
	//uint8_t APDU_BtL_04[5] = {0x80, 0x0C, 0x00, 0x00, 0x00};
	//uint8_t APDU_BtL_05[21] = {0x80, 0x3E, 0x00, 0x00, 0x10, 0xAD, 0xB9, 0xF7, 0x53, 0xB0, 0xE5, 0x99, 0xFC, 0xC4, 0x6E, 0xEE, 0xB5, 0xDC, 0xD8, 0xB6, 0x5A};
	//uint8_t APDU_BtL_06[6] = {0x80, 0x40, 0x00, 0x00, 0x01, 0xFF};
	//uint8_t APDU_BtL_07[6] = {0x80, 0xB0, 0x00, 0x00, 0x01, 0x96};
	//uint8_t APDU_BtL_08[21] = {0x80, 0x88, 0x00, 0x00, 0x10, 0xFF, 0x65, 0xC1, 0x22, 0x70, 0x43, 0x52, 0x61, 0xC4, 0x6E, 0xEE, 0xB5, 0xDC, 0xD8, 0xB6, 0x5A};
	//uint8_t APDU_BtL_09[6] = {0x80, 0x58, 0x00, 0x00, 0x01, 0x00};
	//uint8_t APDU_BtL_10[6] = {0x80, 0xA0, 0x00, 0x00, 0x01, 0xFF};

	uint8_t CSN[8] = {0x00,};
	
	uint8_t temp[8] = {0xA1, 0x14, 0x03, 0x45, 0x3C, 0x3D, 0x19, 0x4A};
	uint8_t temp_2[4] = {0x53, 0xDA, 0x0E, 0x94};
	uint8_t temp_3[4] = {0x9A, 0x02, 0x15, 0xDF};
	uint8_t temp_Key[24] = {0x00,};
	uint8_t DataXOR[8] = {0x00,};
	uint8_t MAC_Data[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xD6, 0x00, 0x01, 0x08, 0x80, 0x00, 0x00};
	uint8_t MAC[16] = {0x00,};
	//uint8_t APDU_Install[39] = {0x80, 0xE6, 0x0C, 0x00, 0x22, 0x06, 0xA0, 0x00, 0x47, 0x49, 0x54, 0x10, 0x07, 0xA0, 0x00, 0x47, 0x49, 0x54, 0x10, 0x01, 0x07, 0xA0, 0x00, 0x47, 0x49, 0x54, 0x10, 0x01, 0x01, 0x10, 0x07, 0xC9, 0x05, 0x04, 0x46, 0x07, 0x02, 0x5A, 0x00};

	/*
    se_close();
	se_open();
	
	// Back to Load
	// 00A4040010 6248F6C561573528D48044D77BF8AEC7
	memcpy(apdu, APDU_BtL_01, 21);
	send_APDU(apdu, 21, resp, sw); // 9000
	
	// 80BB000000
    if (sw[0] == 0x90 && sw[1] == 0x00)
    
{        memcpy(apdu, APDU_BtL_02, 5);
        send_APDU(apdu, 5, resp, sw); // 9000 or 6981 or 6982
    }
    else return HSM_UNKNOWN_ERROR;
    */
    
    g_HSM_update_flag = HSM_NORMAL_MODE;
	se_close();
	se_open();
	se_close();
	se_open();

	// OS pre load
	/*
	memcpy(apdu, APDU_BtL_03, 21);
	send_APDU(apdu, 21, resp, sw);
	
	memcpy(apdu, APDU_BtL_04, 5);
	send_APDU(apdu, 5, resp, sw);
	
	memcpy(apdu, APDU_BtL_05, 21);
	send_APDU(apdu, 21, resp, sw);
	
	memcpy(apdu, APDU_BtL_06, 6);
	send_APDU(apdu, 6, resp, sw);
	
	memcpy(apdu, APDU_BtL_07, 6);
	send_APDU(apdu, 6, resp, sw);
	*/

	U8 Retry = 0;

	// OS pre load
	for(i = 0 ; i < 5 ; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 257))
		{			
            Retry++;
			if(Retry > 3) break;
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		GLogN(".");
        
        if(i%2 == 0)
        {
            HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
        }
        
        osDelay(1);
	}
	
    g_HSM_update_flag = HSM_SEND_HSM_DATA;
    
	se_close();
	se_open();

	// Send Update Data
	for(i = 5 ; i < APDU_blocknum - 3; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 257))
		{			
            Retry++;
			if(Retry > 3) return HSM_UNKNOWN_ERROR;
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		if (sw[0] != 0x90 && sw[1] != 0x00)	 return HSM_UNKNOWN_ERROR;
		
		GLogN(".");
        
        pHSMAck->mProgress = (i*68)/APDU_blocknum;
        
        if(i%15 == 0)
        {
            HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
        }
        
        osDelay(1);
	}
	
    //g_HSM_update_flag = 1;
	//free(pUpdate_APDU);
	se_close();
	se_open();
    se_close();
	se_open();

	/*
	memcpy(apdu, APDU_BtL_08, 21);
	send_APDU(apdu, 21, resp, sw);
	
	memcpy(apdu, APDU_BtL_09, 6);
	send_APDU(apdu, 6, resp, sw);
	
	memcpy(apdu, APDU_BtL_10, 6);
	send_APDU(apdu, 6, resp, sw);
	*/
    pHSMAck->mProgress = 69;
	for(i = APDU_blocknum -3 ; i < APDU_blocknum ; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 257))
		{			
            Retry++;
			if(Retry > 3) 
            {
              return HSM_UNKNOWN_ERROR;
            }
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		GLogN(".");
        
        if(i%5 == 0)
        {
            HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
            osDelay(10);
        }
        
        osDelay(1);
	}
    
	pHSMAck->mProgress = 70;
	se_close();
	se_open();
	
	// preperso
	// Get Data version number
	apdu[0] = 0x80;
	apdu[1] = 0xD6;
	apdu[2] = 0x00;
	apdu[3] = 0x00;
	apdu[4] = 0x08;
		
	send_APDU(apdu, 0x05, resp, sw);
	memcpy(CSN, resp, 8);
    
    HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
	
	for(i = 0 ; i < 8 ; i++)
		DataXOR[i] = temp[i] ^ CSN[i];
	
	memcpy(temp_Key, DataXOR, 2);
	memcpy(&temp_Key[2], temp_2, 4);
	
	memcpy(temp, &DataXOR[2], 4);
	memcpy(&temp_Key[6], temp, 4);
	
	memcpy(&temp_Key[10], temp_3, 4);
	memcpy(temp, &DataXOR[6], 2);
	
	memcpy(&temp_Key[14], temp, 2);
	
	memcpy(MAC_Data, CSN, 8);
	
    memcpy(&temp_Key[16], temp_Key, 8);
    
	// Generate MAC
	TDES_CBC(MAC_Data, temp_Key, MAC, 16);
    memcpy(apdu, &MAC_Data[8], 5);
    memcpy(&apdu[5], &MAC[8], 8);
	send_APDU(apdu, 13, resp, sw);
	
	
	/* 20240328 
	MutualAuthHSM();
	
	
	// Install for Install 
	memcpy(apdu, APDU_Install, 39);
	send_APDU(apdu, 39, resp, sw);
	*/
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
	
}

int BackToLoadHSM(uint8_t *ValATR)	// Clear OS
{	
	uint8_t APDU_BtL_01[21] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0x62, 0x48, 0xF6, 0xC5, 0x61, 0x57, 0x35, 0x28, 0xD4, 0x80, 0x44, 0xD7, 0x7B, 0xF8, 0xAE, 0xC7};
	uint8_t APDU_BtL_02[5] = {0x80, 0xBB, 0x00, 0x00, 0x00};
	uint8_t ret = 0;

	// Back to Load
	// 00A4040010 6248F6C561573528D48044D77BF8AEC7
	memcpy(apdu, APDU_BtL_01, 21);
	send_APDU(apdu, 21, resp, sw); // 9000
	
	// 80BB000000
    if (sw[0] == 0x90 && sw[1] == 0x00)
    {
        memcpy(apdu, APDU_BtL_02, 5);
        send_APDU(apdu, 5, resp, sw); // 9000 or 6981 or 6982

		//if (sw[0] == 0x90 && sw[1] == 0x00)	
		{
            // HSM Reset
            InitHSMUart( ISO7816_BAUDRATE );
            clearHSMReceiveData();
            HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

            ColdResetHSM(0);
            osDelay( 100 );
            clearHSMReceiveData();
            ColdResetHSM(1);
            osDelay( 100 );
			
			// ATR 
			ret = read_bytes_atr( ValATR, HSM_ATR_LEN ); //ATR

			if(ret == 0) return HSM_UNKNOWN_ERROR;
			else return HSM_SUCCESS;
		}
		//else return HSM_UNKNOWN_ERROR;
    }
	else if ((sw[0] == 0x00 && sw[1] == 0x6E)||(sw[0] == 0x6E && sw[1] == 0x00))
    {        
        //memcpy(apdu, APDU_BtL_02, 5);
        //send_APDU(apdu, 5, resp, sw); // 9000 or 6981 or 6982

		//if (sw[0] == 0x90 && sw[1] == 0x00)	
		{			
            // HSM Reset
            InitHSMUart( ISO7816_BAUDRATE );
            clearHSMReceiveData();
            HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

            ResetHSM(0);
            osDelay( 100 );
            clearHSMReceiveData();
            ResetHSM(1);
            osDelay( 100 );
			
			// ATR 
			ret = read_bytes_atr( ValATR, HSM_ATR_LEN ); //ATR

			if(ret == 0) return HSM_UNKNOWN_ERROR;
			else return HSM_SUCCESS;
		}
		//else return HSM_UNKNOWN_ERROR;
    }
    else return HSM_UNKNOWN_ERROR;
}


// AID90 : 80E60C001C05A00000009007A000000090010207A0000000900102010002C90000
int ASKAppletLoadHSM_AID90(uint8_t APDU_blocknum)	
{	
    int i = 0;
    U8 Retry = 0;
    U32 uiCalcProgress = 0;
	
	uint8_t APDU_Install[33] = {0x80, 0xE6, 0x0C, 0x00, 0x1C, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x90, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x90, 0x01, 0x02, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x90, 0x01, 0x02, 0x01, 0x00, 0x02, 0xC9, 0x00, 0x00};
	uint8_t APDU_Transaction_01[21] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0x40, 0x08, 0x11, 0xE4, 0x10, 0x28, 0xE0, 0x20, 0x80, 0x20, 0x60, 0x08, 0x2C, 0x4A, 0x93, 0xA4};
	uint8_t APDU_Transaction_02[5] = {0x80, 0xA8, 0x00, 0x17, 0x00};
	uint8_t APDU_Transaction_03[6] = {0x80, 0x32, 0x7B, 0x4F, 0x01, 0x20};
    
    uiCalcProgress = pHSMAck->mProgress;
	
	// 20240610
	// Send these commands after Reset
	
    // HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
    clearHSMReceiveData();
             
	memcpy(apdu, APDU_Transaction_01, 21);
	send_APDU(apdu, 21, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
		
	memcpy(apdu, APDU_Transaction_02, 5);
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	memcpy(apdu, APDU_Transaction_03, 6);
	send_APDU(apdu, 6, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	// HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
  
	// 20240328 
	MutualAuthHSM();
   
    for(i = 0 ; i < APDU_blocknum ; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 260))
		{			
            Retry++;
			if(Retry > 3) return HSM_UNKNOWN_ERROR;
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		GLogN(".");
        
        pHSMAck->mProgress = uiCalcProgress + ((i*6)/APDU_blocknum);
        
        if(i%10 == 0)
        {
#if 0
            U8 data[2] = {0x00, 0x5D};
            U8 out[20] = {0x00, };
            U32 OutputMessageLength = 0;
#if 0          
            //Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ); 
#endif
			//Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            while(1)
            {
                if(getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                {
                    if(getAESDecoding_ECB(out, OutputMessageLength, g_ucAES256_Key, AES256, out, OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                    {
                        if((data[0] == out[0]) && (data[1] == out[1])) 
                        {
                            break;
                        }
                    }
                }
            }
#endif
			HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
        }
        
        osDelay(1);
	}
	
	MutualAuthHSM();	
	
	// Install for Install 
	memcpy(apdu, APDU_Install, 33);
	send_APDU(apdu, 33, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
}

// AID92 : 80E60C001C05A00000009207A000000092010207A0000000920102010002C90000
int ASKAppletLoadHSM_AID92(uint8_t APDU_blocknum)	
{	
    int i = 0;
    U8 Retry = 0;
    U32 uiCalcProgress = 0;
	
	uint8_t APDU_Install[33] = {0x80, 0xE6, 0x0C, 0x00, 0x1C, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x92, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x92, 0x01, 0x02, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x92, 0x01, 0x02, 0x01, 0x00, 0x02, 0xC9, 0x00, 0x00};
	uint8_t APDU_Transaction_01[21] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0x40, 0x08, 0x11, 0xE4, 0x10, 0x28, 0xE0, 0x20, 0x80, 0x20, 0x60, 0x08, 0x2C, 0x4A, 0x93, 0xA4};
	uint8_t APDU_Transaction_02[5] = {0x80, 0xA8, 0x00, 0x17, 0x00};
	uint8_t APDU_Transaction_03[6] = {0x80, 0x32, 0x7B, 0x4F, 0x01, 0x20};
	
    uiCalcProgress = pHSMAck->mProgress;
    
	// 20240610
	// Send these commands after Reset
	
    // HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
    clearHSMReceiveData();
             
	memcpy(apdu, APDU_Transaction_01, 21);
	send_APDU(apdu, 21, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
		
	memcpy(apdu, APDU_Transaction_02, 5);
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	memcpy(apdu, APDU_Transaction_03, 6);
	send_APDU(apdu, 6, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	// HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
  
	// 20240328 
	MutualAuthHSM();
   
    for(i = 0 ; i < APDU_blocknum ; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 260))
		{			
            Retry++;
			if(Retry > 3) return HSM_UNKNOWN_ERROR;
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		GLogN(".");
        
        pHSMAck->mProgress = uiCalcProgress + ((i*6)/APDU_blocknum);
        
        if(i%10 == 0)
        {
#if 0
            U8 data[2] = {0x00, 0x5D};
            U8 out[20] = {0x00, };
            U32 OutputMessageLength = 0;
#if 0          
            //Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ); 
#endif
			//Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            while(1)
            {
                if(getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                {
                    if(getAESDecoding_ECB(out, OutputMessageLength, g_ucAES256_Key, AES256, out, OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                    {
                        if((data[0] == out[0]) && (data[1] == out[1])) 
                        {
                            break;
                        }
                    }
                }
            }
#endif
			HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
        }
        
        osDelay(1);
	}
	
	MutualAuthHSM();	
	
	// Install for Install 
	memcpy(apdu, APDU_Install, 33);
	send_APDU(apdu, 33, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
}

// AID93 : 80E60C001C05A00000009307A000000093010207A0000000930102010002C90000
int ASKAppletLoadHSM_AID93(uint8_t APDU_blocknum)	
{	
    int i = 0;
    U8 Retry = 0;
    U32 uiCalcProgress = 0;
	
	uint8_t APDU_Install[33] = {0x80, 0xE6, 0x0C, 0x00, 0x1C, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x93, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x93, 0x01, 0x02, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x93, 0x01, 0x02, 0x01, 0x00, 0x02, 0xC9, 0x00, 0x00};
	uint8_t APDU_Transaction_01[21] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0x40, 0x08, 0x11, 0xE4, 0x10, 0x28, 0xE0, 0x20, 0x80, 0x20, 0x60, 0x08, 0x2C, 0x4A, 0x93, 0xA4};
	uint8_t APDU_Transaction_02[5] = {0x80, 0xA8, 0x00, 0x17, 0x00};
	uint8_t APDU_Transaction_03[6] = {0x80, 0x32, 0x7B, 0x4F, 0x01, 0x20};
	
    uiCalcProgress = pHSMAck->mProgress;
    
	// 20240610
	// Send these commands after Reset
	
    // HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
    clearHSMReceiveData();
             
	memcpy(apdu, APDU_Transaction_01, 21);
	send_APDU(apdu, 21, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
		
	memcpy(apdu, APDU_Transaction_02, 5);
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	memcpy(apdu, APDU_Transaction_03, 6);
	send_APDU(apdu, 6, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	// HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
  
	// 20240328 
	MutualAuthHSM();
   
    for(i = 0 ; i < APDU_blocknum ; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 260))
		{			
            Retry++;
			if(Retry > 3) return HSM_UNKNOWN_ERROR;
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		GLogN(".");
        
        pHSMAck->mProgress = uiCalcProgress + ((i*6)/APDU_blocknum);
        
        if(i%10 == 0)
        {
#if 0
            U8 data[2] = {0x00, 0x5D};
            U8 out[20] = {0x00, };
            U32 OutputMessageLength = 0;
#if 0          
            //Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ); 
#endif
			//Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            while(1)
            {
                if(getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                {
                    if(getAESDecoding_ECB(out, OutputMessageLength, g_ucAES256_Key, AES256, out, OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                    {
                        if((data[0] == out[0]) && (data[1] == out[1])) 
                        {
                            break;
                        }
                    }
                }
            }
#endif
			HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
        }
        
        osDelay(1);
	}
	
	MutualAuthHSM();	
	
	// Install for Install 
	memcpy(apdu, APDU_Install, 33);
	send_APDU(apdu, 33, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
}

// AID94 : 80E60C001C05A00000009407A000000094010207A0000000940102010002C90000
int ASKAppletLoadHSM_AID94(uint8_t APDU_blocknum)	
{	
    int i = 0;
    U8 Retry = 0;
    U32 uiCalcProgress = 0;
	
	uint8_t APDU_Install[33] = {0x80, 0xE6, 0x0C, 0x00, 0x1C, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x94, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x94, 0x01, 0x02, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x94, 0x01, 0x02, 0x01, 0x00, 0x02, 0xC9, 0x00, 0x00};
	uint8_t APDU_Transaction_01[21] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0x40, 0x08, 0x11, 0xE4, 0x10, 0x28, 0xE0, 0x20, 0x80, 0x20, 0x60, 0x08, 0x2C, 0x4A, 0x93, 0xA4};
	uint8_t APDU_Transaction_02[5] = {0x80, 0xA8, 0x00, 0x17, 0x00};
	uint8_t APDU_Transaction_03[6] = {0x80, 0x32, 0x7B, 0x4F, 0x01, 0x20};
	
    uiCalcProgress = pHSMAck->mProgress;
    
	// 20240610
	// Send these commands after Reset
	
    // HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
    clearHSMReceiveData();
             
	memcpy(apdu, APDU_Transaction_01, 21);
	send_APDU(apdu, 21, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
		
	memcpy(apdu, APDU_Transaction_02, 5);
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	memcpy(apdu, APDU_Transaction_03, 6);
	send_APDU(apdu, 6, resp, sw);
	
	if (sw[0] != 0x90 && sw[1] != 0x00)	return HSM_UNKNOWN_ERROR;
	
	// HSM Reset
    InitHSMUart( ISO7816_BAUDRATE );
    clearHSMReceiveData();
    HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );

    ResetHSM(0);
    osDelay( 100 );
    clearHSMReceiveData();
    ResetHSM(1);
    osDelay( 100 );
  
	// 20240328 
	MutualAuthHSM();
   
    for(i = 0 ; i < APDU_blocknum ; i++)
	{		
        Retry = 0;
		while(Send_HSM_File(apdu, i, 260))
		{			
            Retry++;
			if(Retry > 3) return HSM_UNKNOWN_ERROR;
		}
		send_APDU(apdu, apdu[4] + 5, resp, sw);
		GLogN(".");
        
        pHSMAck->mProgress = uiCalcProgress + ((i*6)/APDU_blocknum);
        
        if(i%10 == 0)
        {
#if 0
            U8 data[2] = {0x00, 0x5D};
            U8 out[20] = {0x00, };
            U32 OutputMessageLength = 0;
#if 0          
            //Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ); 
#endif
			//Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
            while(1)
            {
                if(getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                {
                    if(getAESDecoding_ECB(out, OutputMessageLength, g_ucAES256_Key, AES256, out, OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
                    {
                        if((data[0] == out[0]) && (data[1] == out[1])) 
                        {
                            break;
                        }
                    }
                }
            }
#endif
			HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
        }
        
        osDelay(1);
	}
	
	MutualAuthHSM();	
	
	// Install for Install 
	memcpy(apdu, APDU_Install, 33);
	send_APDU(apdu, 33, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
}

int ASKAppletDeleteHSM(void)	
{	
	// 80E40080074F05A000000090
	uint8_t APDU_Delete[12] = {0x80, 0xE4, 0x00, 0x80, 0x07, 0x4F, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x90};

	// Send these commands after Reset 
	// 20240610 
	MutualAuthHSM();
	
  // Delete  
	memcpy(apdu, APDU_Delete, 12);
	send_APDU(apdu, 12, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_NOT_RESPONSE;
}

// AID : 0x90
int ASKSignHSM_AID90(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign)	
{	
	// uint8_t APDU_ASKSign[5] = {0x90, 0x38, 0x00, 0x00, 0x28};
	
	// ASK SIGN
	// 9038000010 Data(16byte)
	apdu[0] = 0x90;
	apdu[1] = 0x38;
	apdu[2] = 0x00;
	apdu[3] = 0x00;
	apdu[4] = 0x28;
	memcpy(&apdu[5], Seed, 8);
	memcpy(&apdu[13], EnECU, 16);
	memcpy(&apdu[29], IV, 16);
	
	send_APDU(apdu, 0x28 + 0x05, resp, sw); // 9000
	
	memcpy(ValSign, resp, 8);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_UNKNOWN_ERROR;
}
// AID : 0x92
int ASKSignHSM_AID92(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign)	
{	
	// uint8_t APDU_ASKSign[5] = {0x90, 0x38, 0x00, 0x00, 0x28};
	
	// ASK SIGN
	// 9038000010 Data(16byte)
	apdu[0] = 0x90;
	apdu[1] = 0x38;
	apdu[2] = 0x00;
	apdu[3] = 0x01;
	apdu[4] = 0x28;
	memcpy(&apdu[5], Seed, 8);
	memcpy(&apdu[13], EnECU, 16);
	memcpy(&apdu[29], IV, 16);
	
	send_APDU(apdu, 0x28 + 0x05, resp, sw); // 9000
	
	memcpy(ValSign, resp, 8);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_UNKNOWN_ERROR;
}
// AID : 0x93
int ASKSignHSM_AID93(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign)	
{	
	// uint8_t APDU_ASKSign[5] = {0x90, 0x38, 0x00, 0x00, 0x28};
	
	// ASK SIGN
	// 9038000010 Data(16byte)
	apdu[0] = 0x90;
	apdu[1] = 0x38;
	apdu[2] = 0x00;
	apdu[3] = 0x02;
	apdu[4] = 0x28;
	memcpy(&apdu[5], Seed, 8);
	memcpy(&apdu[13], EnECU, 16);
	memcpy(&apdu[29], IV, 16);
	
	send_APDU(apdu, 0x28 + 0x05, resp, sw); // 9000
	
	memcpy(ValSign, resp, 8);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_UNKNOWN_ERROR;
}

// AID : 0x94
int ASKSignHSM_AID94(uint8_t *Seed, uint8_t *EnECU, uint8_t *IV, uint8_t *ValSign)	
{	
	// uint8_t APDU_ASKSign[5] = {0x90, 0x38, 0x00, 0x00, 0x28};
	
	// ASK SIGN
	// 9038000010 Data(16byte)
	apdu[0] = 0x90;
	apdu[1] = 0x38;
	apdu[2] = 0x00;
	apdu[3] = 0x03;
	apdu[4] = 0x28;
	memcpy(&apdu[5], Seed, 8);
	memcpy(&apdu[13], EnECU, 16);
	memcpy(&apdu[29], IV, 16);
	
	send_APDU(apdu, 0x28 + 0x05, resp, sw); // 9000
	
	memcpy(ValSign, resp, 8);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)	return HSM_SUCCESS;
	else return HSM_UNKNOWN_ERROR;
}
// **********************************
// API functions
// **********************************
int DeactivationHSM(void)
{	
    se_close();
	return HSM_SUCCESS;
}


int ActivationHSM(void)
{	//uint8_t apdu[256];
	//uint8_t resp[256];
	//uint8_t sw[2];
	
	se_open();
		
	// 00A4040007 A0004749541001
	apdu[0] = 0x00;
	apdu[1] = 0xA4;
	apdu[2] = 0x04;
	apdu[3] = 0x00;
	apdu[4] = 0x07;

	apdu[5] = 0xA0;
	apdu[6] = 0x00;
	apdu[7] = 0x47;
	apdu[8] = 0x49;
	apdu[9] = 0x54;
	apdu[10] = 0x10;
	apdu[11] = 0x01;

	send_APDU(apdu, 12, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        return HSM_SUCCESS;
	}	
	else return HSM_NOT_RESPONSE;
}


int ReadCSNHSM(uint8_t *CSN)

{	//uint8_t apdu[256];
	//uint8_t sw[2];
	
	// 00CA010108
	apdu[0] = 0x00;
	apdu[1] = 0xCA;
	apdu[2] = 0x01;
	apdu[3] = 0x01;
	apdu[4] = 0x08;

	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        memcpy(CSN, resp, 8);
		return HSM_SUCCESS;
	}
	else return HSM_NOT_RESPONSE;
}


int InternalAuthHSM(int KeyId, uint8_t *SR, uint8_t *CR, uint8_t *Sign1)
{	//uint8_t apdu[256];
	//uint8_t resp[256];
	//uint8_t sw[2];	
	
	// 008800xx08
	apdu[0] = 0x00;
	apdu[1] = 0x88;
	apdu[2] = 0x00;
	apdu[3] = KeyId;
	apdu[4] = 0x08;
	memcpy(&apdu[5], SR, 8);
	send_APDU(apdu, 13, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        memcpy(CR, resp, 8);
		memcpy(Sign1, &resp[8], 16);
		return HSM_SUCCESS;
	}
	else
	{		
        if (sw[0] == 0x69 &&sw[1] == 0x85) return HSM_NO_AUTHKEY_INDEX;
		else return HSM_NOT_RESPONSE;
	}	
}

int ExternalAuthHSM(int KeyId, uint8_t *Sign2)
{	
    //uint8_t apdu[256];
	//uint8_t resp[256];
	//uint8_t sw[2];
	
	// 008200xx10
	apdu[0] = 0x00;
	apdu[1] = 0x82;
	apdu[2] = 0x00;
	apdu[3] = KeyId;
	apdu[4] = 0x10;
	memcpy(&apdu[5], Sign2, 16);
	send_APDU(apdu, 21, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        return HSM_SUCCESS;
	}
	else	
    {		
        if (sw[0] == 0x69 &&sw[1] == 0x85) return HSM_NEDD_INTERNAL_AUTH;
		if (sw[0] == 0x69 && sw[1] == 0x88) return HSM_FAIL_EXTERNAL_AUTH;
		else return HSM_NOT_RESPONSE;
	}	
}

int ReadPublicKeyHSM(uint8_t *KpuHSM, int *KeyLen, uint8_t key_num)
{	
    uint8_t getData[10];
	
	// get data (HSM Public Key Modules)
	getData[0] = 0x00;
	getData[1] = 0xCA;
	getData[2] = 0x01;
	getData[3] = 0xF0 | key_num;
	getData[4] = 0x80;
	send_APDU(getData, 5, resp, sw);
	
	if (sw[0] == 0x69 && sw[1] == 0x83)
	{		
        if(key_num == 0x00)
		{			
            // 9046000100 Generate 1024
			apdu[0] = 0x90;
			apdu[1] = 0x46;
			apdu[2] = 0x00;
			apdu[3] = 0x01;
			apdu[4] = 0x00;
			send_APDU(apdu, 5, resp, sw);
		}
        // get data
        send_APDU(getData, 5, resp, sw);
	}
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
    {		
        memcpy(KpuHSM, resp, 128);
		
		// get data (HSM Public Key Exponent)
		getData[2] = 0x02;
		getData[4] = 0x03;
		send_APDU(getData, 5, resp, sw);
		
		memcpy(&KpuHSM[128],resp, 3);
		*KeyLen = 131;
		return HSM_SUCCESS;
	}
	else return HSM_NOT_RESPONSE;
}

int StoreAuthKeyHSM(uint8_t *EAuthKey, int KeyLen)
{	
    ret = StoreKeyHSM(EAuthKey, KeyLen, 0x09);
	if (ret != 0) return ret;
	
	return HSM_SUCCESS;
}

int StoreHIKeyHSM(uint8_t *EHIKey, int KeyLen)
{	
    ret = StoreKeyHSM(EHIKey, KeyLen, 0x0A);
	if (ret != 0) return ret;
	
	return HSM_SUCCESS;
}

int TransTempKeyHSM(uint8_t *ETempKey, int KeyLen)
{	
    ret = StoreKeyHSM(ETempKey, KeyLen, 0x08);
	if (ret != 0) return ret;
	
	return HSM_SUCCESS;
}

int StorePrivateKeyHSM(uint8_t *EPrivateKey, int KeyLen, uint8_t key_num)
{	
    // Public Key Modules N : 256 bytes
	// Public Key Exponent E : 4 bytes
	// Private Key Exponent : 256 bytes
	
	// E(N) 256 + E(E) 16 + E(D) 256 = 528
	
	if (KeyLen != 528) return HSM_INVALID_KEYLENGTH;
	
	// Initialize crypto
	apdu[0] = 0x90;
	apdu[1] = 0x2A;
	apdu[2] = 0x48;	// CBC-0x49 , ECB-0x48
	apdu[3] = 0x88 | key_num; // 0x80, 0x88	// Keyset Number : 1 : 0x89, 2 : 0x8A, 3 : 0x8B, 4 : 0x8C, 5 : 0x8D, 6 : 0x8E, 7 : 0x8F
	apdu[4] = 0x00;
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        // Store public key fair
		apdu[0] = 0x90;
		apdu[1] = 0x3C;
		apdu[2] = 0x12;	// Public Key Modules
		apdu[3] = EPrivateKey[0];
		apdu[4] = 0xFF;
		memcpy(&apdu[5], &EPrivateKey[1], 255);
		
		send_APDU(apdu, 260, resp, sw);		
		if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
		
		// Store public key fair
		apdu[0] = 0x90;
		apdu[1] = 0x3C;
		apdu[2] = 0x11;	// Public Key Exponent
		apdu[3] = 0x00;
		apdu[4] = 0x10;
		memcpy(&apdu[5], &EPrivateKey[256], 16);
		
		send_APDU(apdu, 21, resp, sw);		
		if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
		
		// Store public key fair
		apdu[0] = 0x90;
		apdu[1] = 0x3C;
		apdu[2] = 0x10;	// Private Key Exponent
		apdu[3] = EPrivateKey[256+16];
		apdu[4] = 0xFF;
		memcpy(&apdu[5], &EPrivateKey[256+17], 255);
		
		send_APDU(apdu, 260, resp, sw);		
		if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
		
		return HSM_SUCCESS;
	}
	else if (sw[0] == 0x69 && sw[1] == 0x85) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}

int StoreCertificateHSM(uint8_t *Certificate, int CertLen, uint8_t Certi_num)
{	
	if (CertLen != SIZE_GIT_CERTIFICATE) return HSM_INVALID_REQUEST;
	
	apdu[0] = 0x00;
	apdu[1] = 0xD6;
	apdu[2] = 0x80 | Certi_num; // Certi_num 1~7
	apdu[3] = 0x00;
	apdu[4] = 0xFF;
	memcpy(&apdu[5], &Certificate[0], 255);
	
	send_APDU(apdu, 260, resp, sw);
	if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
	
	apdu[2] = 0x00;
	apdu[3] = 0xFF;
	memcpy(&apdu[5], &Certificate[255], 255);
	send_APDU(apdu, 260, resp, sw);
	if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
	
	apdu[2] = 0x01;
	apdu[3] = 0xFE;
	apdu[4] = 0x5A;
	memcpy(&apdu[5], &Certificate[255+255], 90);
	send_APDU(apdu, 95, resp, sw);
	if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;	
	
	return HSM_SUCCESS;
}

int ReadCertificateHSM(uint8_t *Certificate, int *CertLen, uint8_t Certi_num)
{	
	*CertLen = SIZE_GIT_CERTIFICATE;
	
	apdu[0] = 0x00;
	apdu[1] = 0xB0;
	apdu[2] = 0x80 | Certi_num; // Certi_num 1~7
	apdu[3] = 0x00;
	apdu[4] = 0xFF;
	send_APDU(apdu, 5, resp, sw);
	if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
	memcpy(&Certificate[0], resp, 255);
	
	
	apdu[2] = 0x00;
	apdu[3] = 0xFF;
	send_APDU(apdu, 5, resp, sw);
	if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
	memcpy(&Certificate[255], resp, 255);
	
	apdu[2] = 0x01;
	apdu[3] = 0xFE;
	apdu[4] = 0x5A;
	send_APDU(apdu, 5, resp, sw);
	if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
	memcpy(&Certificate[255+255], resp, 90);	
	
	return HSM_SUCCESS;
}

int SignPrivateHSM(uint8_t *Seed, int SeedLen, uint8_t *Sign, int *SignLen, uint8_t key_num)
{	
    //if (SeedLen != 8) return HSM_INVALID_KEYLENGTH;
	
	// Initialize crypto
	apdu[0] = 0x90;
	apdu[1] = 0x2A;
	apdu[2] = 0x00;
	apdu[3] = 0x00 | (key_num<<4);
	apdu[4] = 0x00;
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        // Perform crypto
		apdu[0] = 0x90;
		apdu[1] = 0x2E;
		apdu[2] = 0x01;
		apdu[3] = 0x00;	//Seed[0];	[256bytes not yet.....]
		apdu[4] = 0xFF;
		
		memset(&apdu[5], 0, 255);
		//memcpy(&apdu[5+255-SeedLen], Seed, SeedLen);
		
        uint8_t SEED_PADDING[] = {0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        memcpy(&apdu[5], &SEED_PADDING[0], 255);
        memcpy(&apdu[5+235], Seed, SeedLen);
        
		send_APDU(apdu, 260, resp, sw);
		
		if (sw[0] == 0x90 && sw[1] == 0x00)
		{			
            memcpy(Sign, resp, 256);
			*SignLen = SIZE_DATA_RSA2048;
			return HSM_SUCCESS;
		}			
		else return HSM_NOT_RESPONSE;
	}
	else if (sw[0] == 0x69 && sw[1] == 0x85) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}

int SignPrivateHSMSHA256(uint8_t *Seed, int SeedLen, uint8_t *Sign, int *SignLen, uint8_t key_num)
{	
    uint8_t hash_result[32];
	//if (SeedLen != 8) return HSM_INVALID_KEYLENGTH;
	
	// HASH AES256
	// Initialize crypto  902A600000
	apdu[0] = 0x90;
	apdu[1] = 0x2A;
	apdu[2] = 0x60;
	apdu[3] = 0x00;
	apdu[4] = 0x00;
	send_APDU(apdu, 5, resp, sw);	
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        // Perform crypto  902E000008
		apdu[0] = 0x90;
		apdu[1] = 0x2E;
		apdu[2] = 0x00;
		apdu[3] = 0x00;	
		apdu[4] = 0x08; // Seed lenth
		
		memcpy(&apdu[5], Seed, SeedLen);	// 8byte
		send_APDU(apdu, 13, resp, sw);
	
		if (sw[0] == 0x90 && sw[1] == 0x00)
		{			
            memcpy(hash_result, resp, 32);	// 32bytes - AES256 result value lenth 
		}
		else return HSM_NOT_RESPONSE;
	}
	
	// RSA Encrypt
	// Initialize crypto
	apdu[0] = 0x90;
	apdu[1] = 0x2A;
	apdu[2] = 0x00;
	apdu[3] = 0x00 | (key_num<<4);
	apdu[4] = 0x00;
	send_APDU(apdu, 5, resp, sw);
	
	if (sw[0] == 0x90 && sw[1] == 0x00)
	{		
        // Perform crypto
		apdu[0] = 0x90;
		apdu[1] = 0x2E;
		apdu[2] = 0x01;
		apdu[3] = 0x00;	//Seed[0];	[256bytes not yet.....]
		apdu[4] = 0xFF;
		
		memset(&apdu[5], 0, 255);
		//memcpy(&apdu[5+255-SeedLen], Seed, SeedLen);
		
        // uint8_t SEED_PADDING[] = {0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t SEED_PADDING[] = {0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        memcpy(&apdu[5], &SEED_PADDING[0], 255);
        memcpy(&apdu[5+223], hash_result, 32);
        
		send_APDU(apdu, 260, resp, sw);
		
		if (sw[0] == 0x90 && sw[1] == 0x00)
		{			
            memcpy(Sign, resp, 256);
			*SignLen = SIZE_DATA_RSA2048;
			return HSM_SUCCESS;
		}			
		else return HSM_NOT_RESPONSE;
	}
	else if (sw[0] == 0x69 && sw[1] == 0x85) return HSM_NEED_MUTUAL_AUTH;
	else return HSM_NOT_RESPONSE;
}


int WriteDataHSM(uint8_t *Data, int DataLen, uint8_t File_num)
{	
	if ((File_num > INDEX_GIT_FILES) || (File_num == 0)) return HSM_INSUFFICIENT_CAPA;
	if (DataLen > SIZE_GIT_FILES)	return HSM_INSUFFICIENT_CAPA;
	
	// command
	apdu[0] = 0x00;
	apdu[1] = 0xD6;
	apdu[2] = 0x90 | File_num;
	apdu[3] = 0x00;
		
    {		
        uint8_t cnt = DataLen / 255;
		int remainder = DataLen % 255;
		int offset = 0;
		uint8_t i =0;
		
		for (i=0 ; i < cnt ; i++)	// 255
		{			
            apdu[4] = 255;
			memcpy(&apdu[5], &Data[offset], 255);

			send_APDU(apdu, 260, resp, sw);
			if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
			
			if (i == 0)	// set offset
			{				
                apdu[2] = 0x00;
				apdu[3] = 0xFF;
			}
			else if (i == 1)
			{				
                apdu[2] = 0x01;
				apdu[3] = 0xFE;
			}
			else if (i == 2)
			{				
                apdu[2] = 0x02;
				apdu[3] = 0xFD;
			}
			
			offset += 255;
		}
		
		apdu[4] = remainder;		
		memcpy(&apdu[5], &Data[offset], remainder);
	
		send_APDU(apdu, 5+remainder, resp, sw);
		if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
	}
	
	return HSM_SUCCESS;
}

int ReadDataHSM(uint8_t *Data, int DataLen, uint8_t File_num)
{	
    if ((File_num > INDEX_GIT_FILES) || (File_num == 0)) return HSM_INSUFFICIENT_CAPA;
	if (DataLen > SIZE_GIT_FILES)	return HSM_INSUFFICIENT_CAPA;
	
	// command
	apdu[0] = 0x00;
	apdu[1] = 0xB0;
	apdu[2] = 0x90 | File_num;	
	apdu[3] = 0x00;
	
    {		
        uint8_t cnt = DataLen / 255;
		int remainder = DataLen % 255;
		int offset = 0;
		uint8_t i =0;
		
		for (i=0 ; i < cnt ; i++)	// 255
		{			
            apdu[4] = 255;
			
			send_APDU(apdu, 5, resp, sw);
			if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
			
			memcpy(&Data[offset], resp, 255);
			
			if (i == 0)	// set offset
			{				
                apdu[2] = 0x00;
				apdu[3] = 0xFF;
			}
			else if (i == 1)
			{				
                apdu[2] = 0x01;
				apdu[3] = 0xFE;
			}
			else if (i == 2)
			{				
                apdu[2] = 0x02;
				apdu[3] = 0xFD;
			}
			
			offset += 255;
		}
		
		apdu[4] = remainder;
		send_APDU(apdu, 5, resp, sw);
		if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;

		memcpy(&Data[offset], resp, remainder);
	}
		
	return HSM_SUCCESS;
}

int DeleteDataHSM(int DataLen, uint8_t File_num)
{	
    if ((File_num > INDEX_GIT_FILES) || (File_num == 0)) return HSM_INSUFFICIENT_CAPA;
	if (DataLen > SIZE_GIT_FILES)	return HSM_INSUFFICIENT_CAPA;
	
	// command
	apdu[0] = 0x00;
	apdu[1] = 0xD6;
	apdu[2] = 0x90 | File_num;	
	apdu[3] = 0x00;
	
	memset(&apdu[5], 0, 255);	// clear
	
	{		
        uint8_t cnt = DataLen / 255;
		int remainder = DataLen % 255;

		uint8_t i =0;
		
		for (i=0 ; i < cnt ; i++)	// 255
		{			
            apdu[4] = 255;

			send_APDU(apdu, 260, resp, sw);
			if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
			
			if (i == 0)	// set offset
			{				
                apdu[2] = 0x00;
				apdu[3] = 0xFF;
			}
			else if (i == 1)
			{				
                apdu[2] = 0x01;
				apdu[3] = 0xFE;
			}
			else if (i == 2)
			{				
                apdu[2] = 0x02;
				apdu[3] = 0xFD;
			}
		}
		
		apdu[4] = remainder;	
		send_APDU(apdu, 5+remainder, resp, sw);
		if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
	}
	
	return HSM_SUCCESS;
}


int GenerateRSAKeypairHSM(void)
{		
    // command
    apdu[0] = 0x90;
    apdu[1] = 0x08;
    apdu[2] = 0x00;
    apdu[3] = 0x01;
    apdu[4] = 0x00;

    send_APDU(apdu, 5, resp, sw);
    if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
        
    return HSM_SUCCESS;
}

int ReadPublicKeyHSM2(uint8_t *Data, uint32_t *Keylen)
{		
    // command
    apdu[0] = 0x90;
    apdu[1] = 0x08;
    apdu[2] = 0x01;
    apdu[3] = 0x01;
    apdu[4] = 0x83;


    send_APDU(apdu, 5, resp, sw);
    if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;

    if(apdu[2] == 0x01)	*Keylen = 0x83;
        
    memcpy(Data, resp, 0x83); // 0x80 bytes : Public Key Modulus, 0x03 bytes : Public Key Exponent

    return HSM_SUCCESS;
}

int DecryptPrivateKeyHSM(uint8_t *EncryptedData, uint8_t *DecryptedData)
{		
    // command
    apdu[0] = 0x90;
    apdu[1] = 0x08;
    apdu[2] = 0x02;
    apdu[3] = 0x01;
    apdu[4] = 0x80;

    memcpy(&apdu[5], EncryptedData, 0x80);
    send_APDU(apdu, 0x85, resp, sw);
    if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
        
    memcpy(DecryptedData, resp, 0x80); 

    return HSM_SUCCESS;
}

int DeleteRSAKeypairHSM(void)
{		
    // After Mutual auth
    // command
    apdu[0] = 0x90;
    apdu[1] = 0x08;
    apdu[2] = 0x03;
    apdu[3] = 0x01;
    apdu[4] = 0x00;

    send_APDU(apdu, 5, resp, sw);
    if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;
        
    return HSM_SUCCESS;
    }

int StoreAES256KeyHSM(uint8_t *EncryptedData, uint8_t KeyNum)
{		
    // command
    apdu[0] = 0x90;
    apdu[1] = 0x08;
    apdu[2] = 0x04;
    apdu[3] = KeyNum; // 01~05
    apdu[4] = 0x80;

    memcpy(&apdu[5], EncryptedData, 0x80);

    send_APDU(apdu, 0x85, resp, sw);


    if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;

    return HSM_SUCCESS;
}

int DecryptAES256HSM(uint8_t *EncryptedData, uint8_t KeyNum, uint8_t *IV)
{		
    // CTR Mode Decryption
    // command
    apdu[0] = 0x90;
    apdu[1] = 0x0A;
    apdu[2] = KeyNum; // 01~05
    apdu[3] = 0x00; 
    apdu[4] = 0x20;

    memcpy(&apdu[5], EncryptedData, 0x10);
    memcpy(&apdu[5+0x10], IV, 0x10);

    send_APDU(apdu, 0x25, resp, sw);
    if (sw[0] != 0x90 || sw[1] != 0x00) return HSM_UNKNOWN_ERROR;

    return HSM_SUCCESS;
}