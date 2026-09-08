/*************************************************************
 * NOTE : git_mqtt_Topic.c
 * Project : Cloud Dignosis Platform
 * Author : Cho Sang Jun
 * Since : 2023.04.13
**************************************************************/
#include "cmsis_os.h"
#include "main.h"

#include "common.h"
#include "typedef.h"
#include "git_ioctl.h"
#include "Firmware.h"
#include "git_mqtt_Topic.h"
#include "git_global.h"
#include <string.h>


/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define LETTER_SLUSH		'/'

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/


void GetTopicPublish(int8_t* pTopicName)
{
	int8_t ucTopicName[TOPIC_LENGTH] = {0,};
	snprintf(ucTopicName, sizeof(ucTopicName),"/CDP/%.17s/%.8s/SM", g_strWifiMacAddress, gsFwInfo.marrucSerialNo);
	
//	for(int i = 0; i < strlen(ucTopicName); i++)
//	{
//		GLogN("%c", ucTopicName[i]);
//	}
//	GLogN("\r\n");
	
	memcpy(pTopicName,ucTopicName,sizeof(ucTopicName));
}
void GetHcTopicPublish(int8_t* pTopicName)
{
	int8_t ucTopicName[TOPIC_LENGTH] = {0,};
	
	snprintf(ucTopicName, sizeof(ucTopicName),"/CDP/%.17s/%.8s/HC", g_strWifiMacAddress, gsFwInfo.marrucSerialNo);

//	for(int i = 0; i < strlen(ucTopicName); i++)
//	{
//		GLogN("%c", ucTopicName[i]);
//	}
//	GLogN("\r\n");
	
	memcpy(pTopicName,ucTopicName,sizeof(ucTopicName));
}

void GetTopicSubscribe(int8_t* pTopicName)
{
	int8_t ucTopicName[TOPIC_LENGTH] = {0,};
	
	snprintf(ucTopicName, sizeof(ucTopicName),"/CDP/%.17s/%.8s/DM", g_strWifiMacAddress, gsFwInfo.marrucSerialNo);

//	for(int i = 0; i < strlen(ucTopicName); i++)
//	{
//		GLogN("%c", ucTopicName[i]);
//	}
//	
//	GLogN("\r\n");
		
	memcpy(pTopicName,ucTopicName,sizeof(ucTopicName));
}



