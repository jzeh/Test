/*************************************************************
 * NOTE : led.c
 *        LED Control
 * Author : Jay Hong
 * Since : 2022.07.25
**************************************************************/

#include "led.h"
#include "sw_timer.h"
#include "firmware.h"
#include "git_rs9116.h"
#include "git_global.h"
#include "buzzer.h"
#include "usb_device.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
static void LED_TimerCallBack( void );
void LED_SetState( eLED_STATE eState, uint32_t unTimeout , uint32_t ucOnOffTime);
eLED_STATE LED_GetState();
BOOL LED_TimerInit( void );
#ifdef NEW_VCI_III
BOOL LED_TimerDeinit( void );
#endif

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
stLED_INFO g_stLEDInfo;

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
 
void LED_SetState( eLED_STATE eState, uint32_t unTimeout , uint32_t ucOnOffTime) // by 100ms
{
	g_stLEDInfo.eState = eState;
	g_stLEDInfo.unTimeout = (unTimeout - (unTimeout%100));
	g_stLEDInfo.unOldTime = Get_Tmr();
	g_stLEDInfo.ucOnOffTime = (ucOnOffTime / 100);
	//LED_ALL_OFF;
}

eLED_STATE LED_GetState()
{
	return g_stLEDInfo.eState;
}

BOOL LED_TimerInit( void )
{
	int ret = 0;
	
	ret = SetSWTimer( 100, eSWTimer_INFINITE, LED_TimerCallBack, TRUE );
	
	if(ret == -1)
		return false;
	
	g_stLEDInfo.ucTimerIndex = ret;
	
	return true;
}
#ifdef NEW_VCI_III
BOOL LED_TimerDeinit( void )
{
	int ret = 0;
	
	ret = SetSWTimer( 100, eSWTimer_INFINITE, LED_TimerCallBack, FALSE );
	
	if(ret == -1)
		return false;
	
	g_stLEDInfo.ucTimerIndex = ret;
	
	return true;
}
#endif

static void LED_TimerCallBack( void )
{
  	static uint8_t ucLED_Cnt = 0;
    static uint8_t ucLED_St = 0;
	
	switch( LED_GetState() )
	{
		case eLED_OFF:
			{
				LED_ALL_OFF;
				ucLED_Cnt = 0;
			}
			break;
		
		case eLED_GENERAL:
			{
				LED_WHITE_ON;
				ucLED_Cnt = 0;
			}
			break;
		
		case eLED_NORMAL:
			{
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
				if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
				  	LED_YELLOW_ON;
                if( gsFwInfo.mucCurrentMode == eApp_ECUUP_COMMON )
                {}
				else
				  	LED_GREEN_ON;
#else
				if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
				  	LED_YELLOW_ON;
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
                if( gsFwInfo.mucCurrentMode == eApp_ECUUP_COMMON )
                {}
#endif
				else
				  	LED_GREEN_ON;
#endif
				ucLED_Cnt = 0;
                
                //if( (g_iUSBConnected != USBD_STATE_CONFIGURED) && (g_ucBtConnected == 0) )
                //{
                //    g_stLEDInfo.eState = eLED_GENERAL;
                //}
			}
			break;
		
		case eLED_DIAG_COMM:
			{
			  	ucLED_Cnt++;
				if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
				{
					if(g_mqtt_isconnected)
						LED_BLUE_TOGGLE;
					else
						LED_GREEN_TOGGLE;
					ucLED_Cnt = 0;
				}

				if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
				{
					LED_ALL_OFF;

					if(g_mqtt_isconnected)
					{
						LED_BLUE_ON;
						g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
					}
					else if(g_ucBtConnected)
					{
						if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
							LED_YELLOW_ON;
						else
							LED_GREEN_ON;
						g_stLEDInfo.eState = eLED_NORMAL;
					}
					else
					{
						LED_WHITE_ON;
						g_stLEDInfo.eState = eLED_GENERAL;
					}
					ucLED_Cnt = 0;
				}
			}
			break;

		case eLED_REC_COMM:
		  	{
			  	ucLED_Cnt++;
				if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
				{
			  		LED_YELLOW_TOGGLE;
					ucLED_Cnt = 0;
				}
				if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
				{
					LED_ALL_OFF;

					if(g_mqtt_isconnected)
					{
						LED_BLUE_ON;
						g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
					}
					else if(g_ucBtConnected)
					{
						if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
							LED_YELLOW_ON;
						else
							LED_GREEN_ON;
						g_stLEDInfo.eState = eLED_NORMAL;
					}
					else
					{
						LED_WHITE_ON;
						g_stLEDInfo.eState = eLED_GENERAL;
					}
					ucLED_Cnt = 0;
				}
			}
			break;

		case eLED_NOTI:
			{
			  	ucLED_Cnt++;
				if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
				{
			  		LED_RED_TOGGLE;
					ucLED_Cnt = 0;
				}
				
			  	if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
				{
				  	Buzzer_Control( eBUZZER_ON, MSEC(100),MSEC(100), 3 );
					g_stLEDInfo.unOldTime =  Get_Tmr();
				}
		  	}
	  		break;
		case eLED_BT_SCAN:
		{
			static bool s_bScanLedTogle = false;
				ucLED_Cnt++;
				if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
				{
					if(s_bScanLedTogle == false )
					{
						LED_RED_ON;
						LED_BLUE_OFF;

						s_bScanLedTogle = true;
					}
					else
					{
						LED_RED_OFF;
						LED_BLUE_ON;

						s_bScanLedTogle = false;
					}

					ucLED_Cnt = 0;
				}
				if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
				{
					LED_ALL_OFF;

					if(g_mqtt_isconnected)
					{
						LED_BLUE_ON;
						g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
					}
					else if(g_ucBtConnected)
					{
						if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
							LED_YELLOW_ON;
						else
							LED_GREEN_ON;
						g_stLEDInfo.eState = eLED_NORMAL;
					}
					else
					{
						LED_WHITE_ON;
						g_stLEDInfo.eState = eLED_GENERAL;
					}
					ucLED_Cnt = 0;
				}
		}

			break;

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        case eLED_RERPROCOMM_RDBI:
            {
                static bool s_bLedTogle = false;
            	ucLED_Cnt++;
    			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
    			{
                    if ( s_bLedTogle == FALSE )
                    {
    				    LED_ALL_OFF;
                        s_bLedTogle = TRUE;
                    }
                    else
                    {
                        LED_YELLOW_ON;
                        s_bLedTogle = FALSE;
                    }
                        
    				ucLED_Cnt = 0;
    			}
    			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
    			{
    		  		LED_ALL_OFF;
    				ucLED_Cnt = 0;

					if(g_mqtt_isconnected)
					{
						LED_BLUE_ON;
						g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
					}
					else if(g_ucBtConnected)
					{
						if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
							LED_YELLOW_ON;
						else
							LED_GREEN_ON;
						g_stLEDInfo.eState = eLED_NORMAL;
					}
					else
					{
						LED_WHITE_ON;
						g_stLEDInfo.eState = eLED_GENERAL;
					}
    			}
            }
            break;

        case eLED_RERPROCOMM_NO_TARGET:
            {
                static bool s_bLedTogle = false;
            	ucLED_Cnt++;
    			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
    			{
                    if ( s_bLedTogle == FALSE )
                    {
    				    LED_ALL_OFF;
                        s_bLedTogle = TRUE;
                    }
                    else
                    {
                        LED_MAGENTA_ON;
                        s_bLedTogle = FALSE;
                    }
                        
    				ucLED_Cnt = 0;
    			}
    			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
    			{
    		  		LED_ALL_OFF;
    				ucLED_Cnt = 0;

					if(g_mqtt_isconnected)
					{
						LED_BLUE_ON;
						g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
					}
					else if(g_ucBtConnected)
					{
						if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
							LED_YELLOW_ON;
						else
							LED_GREEN_ON;
						g_stLEDInfo.eState = eLED_NORMAL;
					}
					else
					{
						LED_WHITE_ON;
						g_stLEDInfo.eState = eLED_GENERAL;
					}
    			}
            }
            break;
#endif    
        case eLED_HSM_ERROR:
        {
            ucLED_Cnt++;

            if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
            {
                LED_ALL_OFF;
                ucLED_St++;

                if( ucLED_St%4 == 1 ) LED_RED_ON;
                else if( ucLED_St%4 == 2 ) LED_GREEN_ON;
                else if( ucLED_St%4 == 3 )LED_BLUE_ON;
                else LED_YELLOW_ON;

                if( ucLED_St > 20 )
                {
                    ucLED_St = 0;
                    LED_ALL_OFF;
					if(g_mqtt_isconnected)
					{
						LED_BLUE_ON;
						g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
					}
					else if(g_ucBtConnected)
					{
						if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
							LED_YELLOW_ON;
						else
							LED_GREEN_ON;
						g_stLEDInfo.eState = eLED_NORMAL;
					}
					else
					{
						LED_WHITE_ON;
						g_stLEDInfo.eState = eLED_GENERAL;
					}
                }

                ucLED_Cnt = 0;
            }

            if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
            {
                Buzzer_Control( eBUZZER_ON, MSEC(100),MSEC(100), 3 );
                g_stLEDInfo.unOldTime =  Get_Tmr();
            }

            //ucLED_Cnt = 0;
            break;
        }

		// 서버 연결됨: Blue ON
		case eLED_SERVER_CONNECTED:
		{
			LED_BLUE_ON;
			ucLED_Cnt = 0;
		}
		break;

		// 서버 페어링 대기중: Blue↔Yellow 500ms 교대 점멸
		case eLED_SERVER_SCAN:
		{
			static bool s_bServerScanToggle = false;
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				if(s_bServerScanToggle == false)
				{
					LED_ALL_OFF;
					LED_BLUE_ON;
					s_bServerScanToggle = true;
				}
				else
				{
					LED_ALL_OFF;
					LED_YELLOW_ON;
					s_bServerScanToggle = false;
				}
				ucLED_Cnt = 0;
			}

			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				LED_ALL_OFF;

				if(g_mqtt_isconnected)
				{
					LED_BLUE_ON;
					g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
				}
				else if(g_ucBtConnected)
				{
					if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
						LED_YELLOW_ON;
					else
						LED_GREEN_ON;
					g_stLEDInfo.eState = eLED_NORMAL;
				}
				else
				{
					LED_WHITE_ON;
					g_stLEDInfo.eState = eLED_GENERAL;
				}
				ucLED_Cnt = 0;
			}
		}
		break;

		// 서버 연결 타임아웃: Yellow 300ms 점멸, 30초 경과시 부저
		case eLED_SERVER_TIMEOUT:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_YELLOW_TOGGLE;
				ucLED_Cnt = 0;
			}

			// 30초마다 부저 (삐삐)
			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				Buzzer_Control( eBUZZER_ON, MSEC(100), MSEC(100), 2 );
				g_stLEDInfo.unOldTime = Get_Tmr();
			}
		}
		break;

		// 단말 미연결 3분 초과: Red 200ms 점멸, 30초 주기 부저
		case eLED_NO_CONN_3MIN:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_RED_TOGGLE;
				ucLED_Cnt = 0;
			}

			// 30초마다 부저 100ms 3번 (삐삐삐)
			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				Buzzer_Control( eBUZZER_ON, MSEC(100), MSEC(100), 3 );
				g_stLEDInfo.unOldTime = Get_Tmr();
			}
		}
		break;

		// 서버 미연결 3분 초과: Yellow 200ms 점멸, 30초 주기 부저
		case eLED_NO_SERVER_3MIN:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_YELLOW_TOGGLE;
				ucLED_Cnt = 0;
			}

			// 30초마다 부저 100ms 3번 (삐삐삐)
			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				Buzzer_Control( eBUZZER_ON, MSEC(100), MSEC(100), 3 );
				g_stLEDInfo.unOldTime = Get_Tmr();
			}
		}
		break;

		// ECU 리프로 에러: Red 상시 ON
		case eLED_ECU_ERROR:
		{
			LED_RED_ON;
			ucLED_Cnt = 0;
		}
		break;

		// ECU 리프로 완료: Blue(MQTT연결)/Green(BT연결)/White(미연결)
		case eLED_ECU_COMPLETE:
		{
			// 패드 연결 상태에 따라 LED 색상 다름
			if(g_mqtt_isconnected)
				LED_BLUE_ON;	// MQTT 연결: Blue
			else if(g_ucBtConnected)
				LED_GREEN_ON;	// BT 연결: Green
			else
				LED_WHITE_ON;	// 미연결: White

			ucLED_Cnt = 0;
		}
		break;

		case eLED_REC_READY:
		{
			LED_YELLOW_ON;
			ucLED_Cnt = 0;
		}
		break;

		case eLED_REC_TRIGGER:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_YELLOW_TOGGLE;
				ucLED_Cnt = 0;
			}

			if( g_stLEDInfo.unTimeout > 0 &&
				Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				LED_ALL_OFF;
				LED_YELLOW_ON;
				ucLED_Cnt = 0;
				g_stLEDInfo.eState = eLED_REC_READY;
			}
		}
		break;

		case eLED_REC_COMM_FAIL:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_RED_TOGGLE;
				ucLED_Cnt = 0;
			}

			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				LED_ALL_OFF;
				ucLED_Cnt = 0;
				g_stLEDInfo.eState = eLED_OFF;
			}
		}
		break;

		// RSSI Signal Strength: 2 blink then back to Blue
		case eLED_RSSI_STRONG:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_BLUE_TOGGLE;
				ucLED_Cnt = 0;
			}
			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				LED_ALL_OFF;
				LED_BLUE_ON;
				ucLED_Cnt = 0;
				g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
			}
		}
		break;

		case eLED_RSSI_MEDIUM:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_YELLOW_TOGGLE;
				ucLED_Cnt = 0;
			}
			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				LED_ALL_OFF;
				LED_BLUE_ON;
				ucLED_Cnt = 0;
				g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
			}
		}
		break;

		case eLED_RSSI_WEAK:
		{
			ucLED_Cnt++;
			if(ucLED_Cnt >= g_stLEDInfo.ucOnOffTime)
			{
				LED_WHITE_TOGGLE;
				ucLED_Cnt = 0;
			}
			if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				LED_ALL_OFF;
				LED_BLUE_ON;
				ucLED_Cnt = 0;
				g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
			}
		}
		break;

		default:
		  	// To make LED allways ON, if at any state.
		  	if( Get_TmrDelta( Get_Tmr(), g_stLEDInfo.unOldTime ) >= g_stLEDInfo.unTimeout )
			{
				LED_ALL_OFF;
				if(g_mqtt_isconnected)
				{
					LED_BLUE_ON;
					g_stLEDInfo.eState = eLED_SERVER_CONNECTED;
				}
				else if(g_ucBtConnected)
				{
					if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
						LED_YELLOW_ON;
					else
						LED_GREEN_ON;
					g_stLEDInfo.eState = eLED_NORMAL;
				}
				else
				{
					LED_WHITE_ON;
					g_stLEDInfo.eState = eLED_GENERAL;
				}
				g_stLEDInfo.unOldTime = Get_Tmr();
			}
			break;
	}
}
