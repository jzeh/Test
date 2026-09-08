/* Includes ------------------------------------------------------------------*/
#include <stdarg.h>

#include "GIT_Util.h"
#include "Modem_comm.h"
#include "Power_Manager.h"
#include "OBD_Controller.h"
#include "OBD_Controller_Get.h"
#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#endif
#include "HdDebug.h"

eDebugMode m_eDebugMode = DEBUG_MODE_NONE;
eDebugLevel m_eDebugLevel = DEBUG_LEVEL_NONE;
eDebugModule m_eDebugModule = DEBUG_MODULES_NONE;

bool CheckDebugOption(unsigned int option);


void SetSysDebugMode(eDebugMode mode)
{
	m_eDebugMode = mode;
}

char GetSysDebugMode()
{
	return m_eDebugMode;
}

void SetSysDebugLevel(eDebugLevel level, int flag)
{
	if( flag == TRUE )
		m_eDebugLevel |= level;
	else
		m_eDebugLevel &= ~level;
}

void SetSysDebugModule(eDebugModule module , int flag)
{
	if( flag == TRUE )
		m_eDebugModule |= module;
	else
		m_eDebugModule &= ~module;
}

bool CheckDebugOption(unsigned int option)
{
	switch(m_eDebugMode)
	{
		case DEBUG_MODE_NONE:
			return FALSE;
			break;
		case DEBUG_MODE_ALL:
			return TRUE;
			break;
		case DEBUG_MODE_LEVEL:

			if( m_eDebugLevel & option )
				return TRUE;

			break;
		case DEBUG_MODE_MODULES:

			if( m_eDebugModule & option )
				return TRUE;

			break;
	}

	return FALSE;
}

#define DEBUG_ID_TIME_LOG

int GetStringfromID(long long mng,char* carrBuffer)
{
    int size = 0;
    switch(mng)
    {
    case DEBUG_MODULES_ASSERT:
        size = 6;
        strncpy(carrBuffer,"ASSERT",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_SYSTEM:
        size = 3;
        strncpy(carrBuffer,"SYS",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_SYSTEM_HADNLER:
        size = 3;
        strncpy(carrBuffer,"HDL",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_OBD:
        size = 3;
        strncpy(carrBuffer,"OBD",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_MODEM:
        size = 3;
        strncpy(carrBuffer,"MDM",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_QUEUE:
        size = 5;
        strncpy(carrBuffer,"QUEUE",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_STORAGE:
        size = 7;
        strncpy(carrBuffer,"STORAGE",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_SENSOR:
        size = 3;
        strncpy(carrBuffer,"SEN",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_FILE_SYSTEM:
        size = 4;
        strncpy(carrBuffer,"FILE",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_HAL:
        size = 3;
        strncpy(carrBuffer,"HAL",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_APP:
        size = 3;
        strncpy(carrBuffer,"APP",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_BLE:
//        size = 3;
//        strncpy(carrBuffer,"BLE",size);
//        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_CLI:
        size = 3;
        strncpy(carrBuffer,"CLI",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_INIT:
        size = 4;
        strncpy(carrBuffer,"INIT",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_GPS:
        size = 3;
        strncpy(carrBuffer,"GPS",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_POWER:
        size = 3;
        strncpy(carrBuffer,"PWR",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_UTIL:
//        size = 4;
//        strncpy(carrBuffer,"UTIL",size);
//        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_SW_TMR:
        size = 3;
        strncpy(carrBuffer,"TMR",size);
        carrBuffer[size]=0;
        break;
    case DEBUG_MODULES_MODEM_COM:
        size = 4;
        strncpy(carrBuffer,"MDMC",size);
        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_OBD_NAVI:
        size = 4;
        strncpy(carrBuffer,"NAVI",size);
        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG:
//        size = 4;
//        strncpy(carrBuffer,"ACTP",size);
//        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_OBD_PERIOD_LOG:
        size = 4;
        strncpy(carrBuffer,"PERI",size);
        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_OBD_ACTUATOR_LOG:
        size = 4;
        strncpy(carrBuffer,"ACTL",size);
        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_OBD_FINISH_LOG:
        size = 4;
        strncpy(carrBuffer,"FINI",size);
        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_OBD_DBPARSING_LOG:
//        size = 4;
//        strncpy(carrBuffer,"DBPA",size);
//        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_OBD_SYSTEM_MSG:
        size = 3;
        strncpy(carrBuffer,"OSM",size);
        carrBuffer[size]=0;
        break;
	case DEBUG_MODULES_PWR:
        size = 3;
        strncpy(carrBuffer,"PWR",size);
        carrBuffer[size]=0;
        break;
#if defined(FEATURE_EXTENSION_BOARD)
    case DEBUG_MODULES_EXTENSION:
        size = 3;
        strncpy(carrBuffer,"EXT",size);
        carrBuffer[size]=0;
        break;
#endif
    case DEBUG_MODULES_OBD_CANFD:
        size = 3;
        strncpy(carrBuffer,"CFD",size);
        carrBuffer[size]=0;
        break;
    }

    return size;
}

#if !defined(FEATURE_BOOTLOADER)
static unsigned long ulPrevTmr = 0;
#endif

void GITDebug(unsigned int mng, char *fmt,...)
{
  if( CheckDebugOption(mng) == FALSE )
	{
		return;
	}

    va_list ap;
    char string[1024];

#if !defined(FEATURE_BOOTLOADER)
    char carrTmp[32];
#endif
    unsigned char uIndex = 0;

    if ( UART_DEBUG == NULL )
    	return;

#if !defined(FEATURE_BOOTLOADER)
	#if defined(DEBUG_ID_TIME_LOG)
	{
		if( ulPrevTmr == 0 )
			ulPrevTmr = Get_Tmr();

		uIndex += GetStringfromID(mng,carrTmp);
		if(mng == DEBUG_MODULES_OBD_DBPARSING_LOG || mng == DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG || mng == DEBUG_MODULES_UTIL || mng == DEBUG_MODULES_BLE) {}
		else
		{
			sprintf(string, "[%s][%06d]",carrTmp, Get_TmrDelta(Get_Tmr(),ulPrevTmr));
			ulPrevTmr = Get_Tmr();
			uIndex += 10;
		}
	}
	#endif
#endif

    va_start(ap,fmt);
    vsprintf(string+uIndex,fmt,ap);

	string[1023] = '\0';

  UartWriteBuf(UART_DEBUG, (unsigned char*)string, strlen(string));

    va_end(ap);

//    printf("%s",string);
}





