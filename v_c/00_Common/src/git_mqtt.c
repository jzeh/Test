/*************************************************************
 * NOTE : git_mqtt.c
 *      Cloud Dignosis Platform
 * Author : Cho Sang Jun
 * Since : 2023.03.21
**************************************************************/
#include "cmsis_os.h"
#include "main.h"


#include "common.h"
#include "typedef.h"
#include "git_ioctl.h"
#include "git_mqtt.h"
#include "Git_pool.h"
#include "Git_protocol.h"
#include "MQTTClient.h"
#include "Rsi_mqtt_client.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

//MQTT Connection ack
#define MQTT_CONTROL_PACKET_TYPE_CONNACK 	0x20
#define MQTT_CONTROL_PACKET_TYPE_PUBLISH 	0x30
#define MQTT_CONTROL_PACKET_TYPE_PUBACK 	0x40
#define MQTT_CONTROL_PACKET_TYPE_PUBREC 	0x50
#define MQTT_CONTROL_PACKET_TYPE_PUBREL 	0x60
#define MQTT_CONTROL_PACKET_TYPE_PUBCOMP 	0x70
//MQTT Subscription ack
#define MQTT_CONTROL_PACKET_TYPE_SUBACK  	0x90
#define MQTT_CONTROL_PACKET_TYPE_UNSUBACK  	0xB0

#define GITPACKET_PAYLOAD_HEADER_SIZE		8
#define GITPACKET_SOF				0x02


#define RSI_MQTT_MODD_PUB_TOPIC "/CDP/A001/Server/Pub"
		


#define MODD_QOS 				0



stMqttReceiveDataBuffer MqttRevData;

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
extern rsi_mqtt_client_info_t *rsi_mqtt_client;
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/

int GetFunctionIDForMqttJson( U8 asciicode1, U8 asciicode2, U8 asciicode3, U8 asciicode4)
{
 	int hexcode;
	U8 hexcode1, hexcode2,hexcode3, hexcode4;

  	if( asciicode1 > 0x40 )
  	{
  		hexcode1 = asciicode1 - 0x37;
  		if( hexcode1 > 0x0F ) hexcode1 = 0x00;
  	}
  	else if( asciicode1 >= 0x30 )
  	{
  		hexcode1 = asciicode1 - 0x30;
  		if( hexcode1 > 0x09 ) hexcode1 = 0x00;
  	}
  	else
  		hexcode1 = 0x00;

  	if( asciicode2 > 0x40 )
  	{
  		hexcode2 = asciicode2 - 0x37;
  		if( hexcode2 > 0x0F ) hexcode2 = 0x00;
  	}
  	else if( asciicode2 >= 0x30 )
  	{
  		hexcode2 = asciicode2 - 0x30;
  		if( hexcode2 > 0x09 ) hexcode2 = 0x00;
  	}
  	else
  		hexcode2 = 0x00;

	
  	if( asciicode3 > 0x40 )
  	{
  		hexcode3 = asciicode3 - 0x37;
  		if( hexcode3 > 0x0F ) hexcode3 = 0x00;
  	}
  	else if( asciicode3 >= 0x30 )
  	{
  		hexcode3 = asciicode3 - 0x30;
  		if( hexcode3 > 0x09 ) hexcode3 = 0x00;
  	}
  	else
  		hexcode3 = 0x00;


  	if( asciicode4 > 0x40 )
  	{
  		hexcode4 = asciicode4 - 0x37;
  		if( hexcode4 > 0x0F ) hexcode4 = 0x00;
  	}
  	else if( asciicode4 >= 0x30 )
  	{
  		hexcode4 = asciicode4 - 0x30;
  		if( hexcode4 > 0x09 ) hexcode4 = 0x00;
  	}
  	else
  		hexcode4 = 0x00;

  	hexcode = (hexcode1<<12)+(hexcode2<<8)+(hexcode3<<4)+(hexcode4);
	
  	return hexcode;
}

int MakeFunctionIDForResponseData( U8 asciicode1, U8 asciicode2)
{
 	int hexcode;
  	hexcode = (asciicode2<<8)+(asciicode1);
//printf("MakeFunctionIDForResponseData Result %d !!! \r\n",hexcode);
	
  	return hexcode;
}

void ConvertHexToString(uint8_t *pOriginalData , int nDataLength , uint8_t *pTransData)
{
	U8 hexcodeLeft, hexcodeRight;
	
	for(int i = 0 ; i < nDataLength ; i++)
	{
		hexcodeLeft = (pOriginalData[i] & 0xF0)>>4;
		hexcodeRight = pOriginalData[i] & 0x0F;

		if(hexcodeLeft < 0x0A)
		{
			pTransData[i*2] = 0x30+	hexcodeLeft;
		}

		else
		{
			pTransData[i*2] = 0x37+	hexcodeLeft;
		}

		
		if(hexcodeRight < 0x0A)
		{
			pTransData[i*2+1]  = 0x30+	hexcodeRight;
		}

		else
		{
			pTransData[i*2+1]  = 0x37+	hexcodeRight;
		}
	}
}

	
void GetHeaderControlPacket(uint8_t ucControlPackByte)
{
	uint8_t ucUpperLevelByte = (ucControlPackByte & 0xF0);

	switch(ucUpperLevelByte)
	{	
		case MQTT_CONTROL_PACKET_TYPE_PUBLISH:
			{
//printf("MQTT_CONTROL_PACKET_TYPE_PUBLISH !!! \r\n");
			}
		break;
		default:
			{
//printf("MQTT_CONTROL_PACKET_TYPE DEFAULT !!! \r\n");
			}			
		break;
	}
}

int GetRemainLength(uint8_t ucValue1, uint8_t ucValue2, uint8_t ucValue3, uint8_t ucValue4)
{
	int nRemainLength = 0;
	U16 u16TempValue = 0;
	uint8_t ucMakingResult = 0;

	if(((ucValue1 & 0x80) == 0x80) && ((ucValue2 & 0x80) == 0x80) && ((ucValue3 & 0x80) == 0x80))
	{
	 	ucMakingResult = ucValue4 & 0x7F;
		nRemainLength |= ucMakingResult;		
		nRemainLength = nRemainLength<<7;

		ucMakingResult = ucValue3 & 0x7F;
		nRemainLength |= ucMakingResult;		
		nRemainLength = nRemainLength<<7;

		ucMakingResult = ucValue2 & 0x7F;
		nRemainLength |= ucMakingResult;		
		nRemainLength = nRemainLength<<7;

		ucMakingResult = ucValue1 & 0x7F;
		nRemainLength |= ucMakingResult; 	
	}
	else if(((ucValue1 & 0x80) == 0x80) && ((ucValue2 & 0x80) == 0x80))
	{
		ucMakingResult = ucValue3 & 0x7F;
		nRemainLength |= ucMakingResult;		
		nRemainLength = nRemainLength<<7;

		ucMakingResult = ucValue2 & 0x7F;
		nRemainLength |= ucMakingResult;		
		nRemainLength = nRemainLength<<7;

		ucMakingResult = ucValue1 & 0x7F;
		nRemainLength |= ucMakingResult; 
	}
	else if((ucValue1 & 0x80) == 0x80)
	{
		ucMakingResult = ucValue2 & 0x7F;
		nRemainLength |= ucMakingResult;		
		nRemainLength = nRemainLength<<7;

		ucMakingResult = ucValue1 & 0x7F;
		nRemainLength |= ucMakingResult; 
	}
	else
	{
		ucMakingResult = ucValue1 & 0x7F;
		nRemainLength |= ucMakingResult; 	
	}
	
	//nTopicLength =  (ucHighValue << 8) | ucLowValue;
//printf("GetRemainLength Result %d !!! \r\n",nRemainLength);
	
	return nRemainLength;
}

uint8_t GetRemainLengthPosition(uint8_t ucValue1 , uint8_t ucValue2,uint8_t ucValue3 , uint8_t ucValue4)
{
	uint8_t ucArrayPosition = 0;

	if(((ucValue1 & 0x80) == 0x80) && ((ucValue2 & 0x80) == 0x80) && ((ucValue3 & 0x80) == 0x80))
	{
		ucArrayPosition = 4;
	}
	else if(((ucValue1 & 0x80) == 0x80) && ((ucValue2 & 0x80) == 0x80))
	{
		ucArrayPosition = 3;
	}
	else if((ucValue1 & 0x80) == 0x80)
	{
		ucArrayPosition = 2;
	}
	else
	{
		ucArrayPosition = 1;
	}
	
	return ucArrayPosition;
}

int GetTopicLength(uint8_t ucValue1)
{
	int nTopicLength = 0;

	nTopicLength = ucValue1;
	
//printf("GetTopicLength Result %d !!! \r\n",nTopicLength);	
	
	return nTopicLength;
}


bool GetPayLoadForMqttJson(U8 *pReceivePayLoad,U8 *pConvertPayLoad)
{

	uint16_t ucLength = 0;
	bool bResult = false;

	for(int i = 0 ; i < 1024 ; i++)
	{
		if(pReceivePayLoad[i] == 0x00)
		{
			break;
		}
		else
			ucLength++;
	}

//printf("Received PayLoad Length : %d \r\n",ucLength);

	for(int i = 0 ; i <ucLength/2 ; i++)
	{
            pConvertPayLoad[i] = GetPayLoadEach(pReceivePayLoad[2*i],pReceivePayLoad[2*i+1]);
	}

	return bResult;

}

uint8_t GetPayLoadEach(U8 asciicode1, U8 asciicode2)
{
	uint8_t ucPayLoadEach = 0;
	U8 hexcode1, hexcode2,hexcode3, hexcode4;

  	if( asciicode1 > 0x40 )
  	{
  		hexcode1 = asciicode1 - 0x37;
  		if( hexcode1 > 0x0F ) hexcode1 = 0x00;
  	}
  	else if( asciicode1 >= 0x30 )
  	{
  		hexcode1 = asciicode1 - 0x30;
  		if( hexcode1 > 0x09 ) hexcode1 = 0x00;
  	}
  	else
  		hexcode1 = 0x00;

	if( asciicode2 > 0x40 )
  	{
  		hexcode2 = asciicode2 - 0x37;
  		if( hexcode2 > 0x0F ) hexcode2 = 0x00;
  	}
  	else if( asciicode2 >= 0x30 )
  	{
  		hexcode2 = asciicode2 - 0x30;
  		if( hexcode2 > 0x09 ) hexcode2 = 0x00;
  	}
  	else
  		hexcode2 = 0x00;
	
	ucPayLoadEach = (hexcode1<<4)+hexcode2;
	
	return ucPayLoadEach;
}

void MakeFromMqttPayLoadTohParsingMsg(uint8_t *pPayLoadBuffer)
{
//printf("MakeFromMqttPayLoadTohParsingMsg Start \r\n");
	stCommPkt	*packet;
	stMsgClst	*message;
	uint32_t eInCommType = PACKET_MQTT;
	uint32_t	uiCopyLen	= 0;

	if(pPayLoadBuffer[0] != GITPACKET_SOF)
	{
		return;
	}

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	//////////////////////////////////////////////////
	//Total Length
	//////////////////////////////////////////////////
	
	uiCopyLen = (pPayLoadBuffer[1] & 0x00FF) + ((pPayLoadBuffer[2] << 8) & 0xFF00);

	//////////////////////////////////////////////////
	//Frame
	//////////////////////////////////////////////////
	
	// Get length
	message->mLen = pPayLoadBuffer[2];
	message->mLen <<= 8;
	message->mLen += pPayLoadBuffer[1];
	
	// Get Mode
	message->mMod = pPayLoadBuffer[3];
	message->mMod <<= 8;
	message->mMod += pPayLoadBuffer[4];
	
	// Get Sequence
	message->mSeq = pPayLoadBuffer[5];

	//////////////////////////////////////////////////
	//PayLoad
	//////////////////////////////////////////////////
	
	packet->mLen = pPayLoadBuffer[7];
	packet->mLen <<= 8;
	packet->mLen += pPayLoadBuffer[6];
	
	packet->mFuncID = pPayLoadBuffer[9];
	packet->mFuncID <<= 8;
	packet->mFuncID += pPayLoadBuffer[8];
	
	packet->mCurFrame = pPayLoadBuffer[11];
	packet->mCurFrame <<= 8;
	packet->mCurFrame += pPayLoadBuffer[10];

	packet->mCS = pPayLoadBuffer[13];
	packet->mCS <<= 8;
	packet->mCS += pPayLoadBuffer[12];

	uiCopyLen -= (5 + GITPACKET_PAYLOAD_HEADER_SIZE);	
	
	//////////////////////////////////////////////////
	//PayLoad Data
	//////////////////////////////////////////////////

	if(uiCopyLen !=0)
	{
		memcpy(	&packet->mData[0],&pPayLoadBuffer[14],uiCopyLen);
	}


	packet->pTarget	= &eInCommType;
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);
	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;


	if(osMessageAvailableSpace(hParsingMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hParsingMsg, (uint32_t)message, osWaitForever );
	}
	
/*
	while( osMessageAvailableSpace( hParsingMsg ) == 0 )
	{
		//printf("osMessageAvailableSpace is True\n");
		osDelay( 1 );
	}

//printf("MakeFromMqttPayLoadTohParsingMsg End \r\n");
	
	osMessagePut( hParsingMsg, (uint32_t)message, osWaitForever );
	//osDelay(50);

	*/
		
}




