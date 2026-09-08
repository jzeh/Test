/*************************************************************
 * NOTE : git_mqtt_PubMsg.c
 *      Cloud Dignosis Platform
 * Author : Cho Sang Jun
 * Since : 2023.04.04
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
#include "git_rs9116.h"
#include "git_mqtt_Topic.h"
/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

#define RSI_MQTT_PUB_REAL_TOPIC "/CDP/sample/DM"
#define MODD_QOS				0




/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
   
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/


void SendMqttPub_Start()
{
	//if ((suback) && pub == 1) 
	{
		MQTTMessage publish_msg;
                  
        	uint8_t publish_Testmessage[] = "abcdef";
		int nTestIndex = 325;
       		char strTest = '1';
        	char *TestMsg = "abc";
		int8_t ucPubTopicName[TOPIC_LENGTH] = {0,};
		
		//uint8_t publish_message[] = "THIS IS MQTT CLIENT DEMO FROM APPLICATION";
		//uint8_t publish_message[] = "{"reqcnt":1,"requests":[{"funcid":"0x0145","payload":"020D008000010800450100004E00032D"}]}";
		//uint8_t publish_message[] = "{\"reqcnt\":1\"requests\"}";
		//uint8_t TestSpecialLetter = ':';
		uint8_t publish_message[256] = "{\"reqcnt\":1,\"requests\":[{\"funcid\":\"0x0000\",\"payload\":\"START\"}]}";
                  
		//!The DUP flag MUST be set to 1 by the Client or Server when it attempts to re-deliver a PUBLISH Packet			
		//!The DUP flag MUST be set to 0 for all QoS 0 messages 		   
		publish_msg.dup = 0;			
		//! This field indicates the level of assurance for delivery of an Application Message. The QoS levels are			  
		//! 0  - At most once delivery			  
		//! 1  - At least once delivery 		   
		//! 2  - Exactly once delivery			  
		publish_msg.qos = MODD_QOS;			  
		//! If the RETAIN flag is set to 1, in a PUBLISH Packet sent by a Client to a Server, the Server MUST store 		   
		//! the Application Message and its QoS, so that it can be delivered to future subscribers whose			
		//! subscriptions match its topic name			  
		publish_msg.retained = 0;			 
		//! Attach paylaod			  
		publish_msg.payload = publish_message;			  
		//! Fill paylaod length 		   
		//publish_msg.payloadlen = sizeof(publish_message);
		publish_msg.payloadlen = strlen((char const*)publish_message);
                
		//! Publish message on the topic			
		//original
		//rsi_mqtt_publish(rsi_mqtt_client, (int8_t *)RSI_MQTT_TOPIC, &publish_msg);			  
		//rsi_mqtt_publish(rsi_mqtt_client, (int8_t *)RSI_MQTT_PUB_REAL_TOPIC, &publish_msg);			

		GetTopicPublish(ucPubTopicName);
		//rsi_mqtt_publish(rsi_mqtt_client_git,ucPubTopicName, &publish_msg);			
		
		//pub = 0;		  
	}

	static int nTimeout = 0;
	static int nTxCount = 0;

	nTxCount += 1024;
	if( Get_Tmr() - nTimeout > 1000 )
	{
		nTimeout = Get_Tmr();
		printf("%s] tx count for 1 seconds : %d\r\n", __func__, nTxCount);

		nTxCount = 0;		
	}
}


void SendMqttPub_ReStart()
{
	//if ((suback) && pub == 1) 
	{
		MQTTMessage publish_msg;
                  
		uint8_t publish_Testmessage[] = "abcdef";
		int nTestIndex = 325;
		char strTest = '1';
		char *TestMsg = "abc";
		int8_t ucPubTopicName[TOPIC_LENGTH] = {0,};
		
		//uint8_t publish_message[] = "THIS IS MQTT CLIENT DEMO FROM APPLICATION";
		//uint8_t publish_message[] = "{"reqcnt":1,"requests":[{"funcid":"0x0145","payload":"020D008000010800450100004E00032D"}]}";
		//uint8_t publish_message[] = "{\"reqcnt\":1\"requests\"}";
		//uint8_t TestSpecialLetter = ':';
		uint8_t publish_message[256] = "{\"reqcnt\":1,\"requests\":[{\"funcid\":\"0x0000\",\"payload\":\"START Re\"}]}";
                  
		//!The DUP flag MUST be set to 1 by the Client or Server when it attempts to re-deliver a PUBLISH Packet			
		//!The DUP flag MUST be set to 0 for all QoS 0 messages 		   
		publish_msg.dup = 0;			
		//! This field indicates the level of assurance for delivery of an Application Message. The QoS levels are			  
		//! 0  - At most once delivery			  
		//! 1  - At least once delivery 		   
		//! 2  - Exactly once delivery			  
		publish_msg.qos = MODD_QOS;			  
		//! If the RETAIN flag is set to 1, in a PUBLISH Packet sent by a Client to a Server, the Server MUST store 		   
		//! the Application Message and its QoS, so that it can be delivered to future subscribers whose			
		//! subscriptions match its topic name			  
		publish_msg.retained = 0;			 
		//! Attach paylaod			  
		publish_msg.payload = publish_message;			  
		//! Fill paylaod length 		   
		//publish_msg.payloadlen = sizeof(publish_message);
		publish_msg.payloadlen = strlen((char const*)publish_message);
                
		//! Publish message on the topic			
		//original
		//rsi_mqtt_publish(rsi_mqtt_client, (int8_t *)RSI_MQTT_TOPIC, &publish_msg);			  
		//rsi_mqtt_publish(rsi_mqtt_client, (int8_t *)RSI_MQTT_PUB_REAL_TOPIC, &publish_msg);			

		GetTopicPublish(ucPubTopicName);
		//rsi_mqtt_publish(rsi_mqtt_client_git,ucPubTopicName, &publish_msg);			
		
		//pub = 0;		  
	}

	static int nTimeout = 0;
	static int nTxCount = 0;

	nTxCount += 1024;
	if( Get_Tmr() - nTimeout > 1000 )
	{
		nTimeout = Get_Tmr();
		printf("%s] tx count for 1 seconds : %d\r\n", __func__, nTxCount);

		nTxCount = 0;		
	}
}



void SendMqttPub_Dummy(int8_t ucDummyCount)
{
	// 변수 선언
	MQTTMessage publish_msg;

        uint8_t publish_Testmessage[] = "abcdef";
        int nTestIndex = 325;
        char strTest = '1';
        char *TestMsg = "abc";
        int8_t ucPubTopicName[TOPIC_LENGTH] = {0,};
        uint8_t datetime[7];
        Get_RTCData(datetime);

	// RTC 데이터 가져오기
	Get_RTCData(datetime);

	// publish_message 작성
	uint8_t publish_message[2024] = {0,};		
	sprintf(publish_message, "{\"reqcnt\":1,\"requests\":[{\"funcid\":\"0xFFFF\",\"payload\":\"Dummy %s\"}]}",
	    "Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hell");

	// MQTT 메시지 설정
	publish_msg.dup = 0;
	publish_msg.qos = MODD_QOS;
	publish_msg.retained = 0;
	publish_msg.payload = publish_message;
	publish_msg.payloadlen = strlen((char const*)publish_message);

	// 토픽 이름 가져오기 및 메시지 발행
	GetTopicPublish(ucPubTopicName);
	//rsi_mqtt_publish(rsi_mqtt_client_git, ucPubTopicName, &publish_msg);
}




void SendMqttPubData(int nDecimalFunctionID, uint8_t *pResponseData, int nDataLength)
{
	MQTTMessage publish_msg;
	int8_t ucPubTopicName[TOPIC_LENGTH] = {0,};
	uint8_t publish_message[1024] = "";
    uint8_t publish_message_2[1024] = "";
	sprintf(publish_message, "{\"reqcnt\":1,\"requests\":[{\"funcid\":\"0x%04x\",\"payload\":\"%s\"}]}", 
		nDecimalFunctionID, pResponseData);
    
    sprintf(publish_message_2, "{\"job_type\" : \"HW_SETTING\", \"vin\" : \"KMHE2411BFA000250\",\"ecu_id\" : \"HB1H\",\"data_count\" : 2,\"data\" : [{\"function_id\" : \"0x1102\", \"payload\" : \"0215018000181001021100003F0C001000000000000010000000020000002000000004000000080000000C000000000000000000000001000000020000000000000008000000000800000000000000000000000000000000000000000000000000000505E2023E000000000000000000000000000000010000000000000000000000000000000000000014000000190000003200000037000000881300000500000014000000E8030000E8030000E80300001E0000002C0100002C010000190000003200000000000000800000001500000014000000640000003200000014000000640000000000000000000000FFFF0000FFFF00000000000000000000000000000000000000000000000000000000000000000000033B\"},{\"function_id\" : \"0x1105\",\"payload\" : \"020E0080000909000511000022000303DE\"}],\"utc_time\" : \"2024-06-04 10:50:21.796\"}", 
		nDecimalFunctionID, pResponseData);

	publish_msg.dup = 0;        
	publish_msg.qos = MODD_QOS;
	publish_msg.retained = 0; 
	publish_msg.payload = publish_message_2;
	publish_msg.payloadlen = strlen((char const*)publish_message_2);

	GetTopicPublish(ucPubTopicName);
	//rsi_mqtt_publish(rsi_mqtt_client_git, ucPubTopicName, &publish_msg);

	static int nTimeout = 0;
	static int nTxCount = 0;

	nTxCount += 1024;
	if (Get_Tmr() - nTimeout > 1000)
	{
		nTimeout = Get_Tmr();
		printf("%s] tx count for 1 seconds : %d\r\n", __func__, nTxCount);
		nTxCount = 0;        
	}
}


extern stMqttReceiveDataBuffer MqttRevData;

void SendMqttPubData_Advanced(int nDecimalFunctionID,uint8_t *pResponseData,int nDataLength)
{
	//if ((suback) && pub == 1) 
	{
		MQTTMessage publish_msg;
		int8_t ucPubTopicName[TOPIC_LENGTH] = {0,};
		uint8_t publish_message[1024] = {0x11, 0x12, 0x00};
		//sprintf( publish_message, "{\"reqcnt\":1,\"command\":\"%s\",\"guid\":\"%s\",\"requests\":[{\"funcid\":\"0x%04x\",\"payload\":\"%s\"}]}",MqttRevData.MqttCommand,MqttRevData.MqttGuid,nDecimalFunctionID,pResponseData);
		
		//!The DUP flag MUST be set to 1 by the Client or Server when it attempts to re-deliver a PUBLISH Packet			
		//!The DUP flag MUST be set to 0 for all QoS 0 messages 		   
		publish_msg.dup = 0;			
		//! This field indicates the level of assurance for delivery of an Application Message. The QoS levels are			  
		//! 0  - At most once delivery			  
		//! 1  - At least once delivery 		   
		//! 2  - Exactly once delivery			  
		publish_msg.qos = MODD_QOS;			  
		//! If the RETAIN flag is set to 1, in a PUBLISH Packet sent by a Client to a Server, the Server MUST store 		   
		//! the Application Message and its QoS, so that it can be delivered to future subscribers whose			
		//! subscriptions match its topic name			  
		publish_msg.retained = 0;			 
		//! Attach paylaod			  
		publish_msg.payload = publish_message;			  
		//! Fill paylaod length 		   
		//publish_msg.payloadlen = sizeof(publish_message);
		publish_msg.payloadlen = strlen((char const*)publish_message);
                
		//! Publish message on the topic			
		//original
		//rsi_mqtt_publish(rsi_mqtt_client, (int8_t *)RSI_MQTT_PUB_REAL_TOPIC, &publish_msg);			  
		GetTopicPublish(ucPubTopicName);
		//rsi_mqtt_publish(rsi_mqtt_client_git,ucPubTopicName, &publish_msg); 	
		//pub = 0;		  
	}

	static int nTimeout = 0;
	static int nTxCount = 0;

	nTxCount += 1024;
	if( Get_Tmr() - nTimeout > 1000 )
	{
		nTimeout = Get_Tmr();
		printf("%s] tx count for 1 seconds : %d\r\n", __func__, nTxCount);

		nTxCount = 0;		
	}
}

void SendMqttPub_ConnectTest(void)
{
	// 변수 선언
	MQTTMessage publish_msg;


    int nTestIndex = 325;
    char strTest = '1';
    char *TestMsg = "abc";
    int8_t ucPubTopicName[TOPIC_LENGTH] = {0,};
    uint8_t datetime[7];
	uint8_t publish_message[2000] = {0};
	
	// RTC 데이터 가져오기
	//Get_RTCData(datetime);

	// publish_message 작성

	sprintf(publish_message, "HelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworld");
	// MQTT 메시지 설정
	publish_msg.dup = 0;
	publish_msg.qos = MODD_QOS;
	publish_msg.retained = 0;
	publish_msg.payload = publish_message;
	publish_msg.payloadlen = strlen(publish_message);

	// 토픽 이름 가져오기 및 메시지 발행
	GetTopicPublish(ucPubTopicName);
	//rsi_mqtt_publish(rsi_mqtt_client_git, ucPubTopicName, &publish_msg);
	osDelay(20);
}
