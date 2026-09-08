/*----------------------------------------------------------------------
 *   MQTT Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_MQTT_H__
#define	__GIT_MQTT_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/

typedef struct _stMqttReceiveDataBuffer
{
	uint8_t MqttIsAdvancedType;
	uint8_t MqttCommand[10];
	uint8_t MqttGuid[100];
	uint8_t MqttCount;
	uint8_t MqttfunctionID[6];
	uint8_t MqttPayLoad[1024];
	uint8_t MqttConvPayLoad[1024];
}stMqttReceiveDataBuffer;

extern stMqttReceiveDataBuffer MqttRevData;

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/

void GetHeaderControlPacket(uint8_t ucControlPackByte);
int MakeFunctionIDForResponseData( U8 asciicode1, U8 asciicode2);
void ConvertHexToString(uint8_t *pOriginalData , int nDataLength , uint8_t *pTransData);
int GetFunctionIDForMqttJson( U8 asciicode1, U8 asciicode2, U8 asciicode3, U8 asciicode4);
int GetRemainLength(uint8_t ucValue1 , uint8_t ucValue2,uint8_t ucValue3 , uint8_t ucValue4);
uint8_t GetRemainLengthPosition(uint8_t ucValue1 , uint8_t ucValue2,uint8_t ucValue3 , uint8_t ucValue4);
int GetTopicLength(uint8_t ucValue1);
bool GetPayLoadForMqttJson(U8 *pReceivePayLoad,U8 *pConvertPayLoad);
uint8_t GetPayLoadEach(U8 asciicode1, U8 asciicode2);
void MakeFromMqttPayLoadTohParsingMsg(uint8_t *pPayLoadBuffer);

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/

#endif // __GIT_MQTT_H__