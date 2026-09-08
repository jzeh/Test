/* Includes ------------------------------------------------------------------*/
#include "AutolinkMessage.h"
#include "AutolinkConfig.h"

#include "Modem_Manager.h"
#include "GIT_Util.h"
#include "Modem_comm.h"
#include "Power_Manager.h"
#include "OBD_Controller.h"
#include "Message_Make.h"
#include "ff.h"

#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "MngQueue.h"
#include "HdDebug.h"

#include <math.h>
#include <time.h>

#include "HandlerRsvEngCtrl.h"
#include "OBD_Controller_Get.h"

#if defined(PROTOCOL18)
#include "OBD_Manager.h"
#endif

#if defined(FEATURE_EXTENSION_BOARD)
#include "SysPsExtendHdEvent.h"
#include "SysPsExtend.h"
#endif

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

extern bool g_bFuelLevelCheckFlag;

//#define ENABLE_GUARD_LOG

// user action setting variable
stUserActionSetting m_stUserActionSetting;

extern stMsgHandlerData m_stMsgHdData;
extern void SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, unsigned char ncValue,bool bHelpInterrupSignal);
extern void SetActiveValetMode(boolean_t bActive);
extern void SetGeofenceSetting(stGeoFenceSetting stGeofenceValue);
extern void DisplayGeofenceSetting();
extern uint32_t GetUTCTime();

void SetGuardSensitivity(stImpulseSensitivity stImpulseSetValue)
{
    Trace("Sensitivity Active Level : %d\n",stImpulseSetValue.ucActiveLevel);
    memcpy((char*)&m_stUserActionSetting.Guard.stImpulseSetting,(char*)&stImpulseSetValue,sizeof(stImpulseSensitivity));
    // low : 70 / middle : 50 / high : 20

    // save setting value
    WriteConfig(false,true);
}


uint8_t GetGuardActiveLevel()
{
	uint8_t ucValue = 0;
    Trace("Guard Active Level : %d\r\n",m_stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel);

	if( m_stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel == 1 )
		ucValue = m_stUserActionSetting.Guard.stImpulseSetting.ucHigh;
	else if( m_stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel == 2 )
		ucValue = m_stUserActionSetting.Guard.stImpulseSetting.ucMiddle;
	else if( m_stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel == 3 )
		ucValue = m_stUserActionSetting.Guard.stImpulseSetting.ucLow;

    if( ucValue < 20 || ucValue > 70 )
        ucValue = 70;

    return ucValue;
}


void SetActiveGuardMode(boolean_t bActive)
{
    Trace("Guard Active : %d\n",bActive);
    m_stUserActionSetting.Guard.bActive = bActive;

    if( bActive == true )
    {
        uint32_t unTime = GetUTCTime();
        DisplayTime("GuideMode Active Time : ",unTime);

        m_stUserActionSetting.Guard.unActiveTime = unTime;
        m_stUserActionSetting.Guard.unKeepTime = 1*60*60; // 1 hour

        // set impulse alram.
        Trace("Guard Active Level : %d\r\n",m_stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel);
        SetupForInterruptforImpulse(false,false,m_stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel,false);

//#warning "When valet is actived, do we need keep alive????"
        // request pospond flag to system
        //SetPostPoneSleepFlag(true);
    }

    // save setting value
    WriteConfig(false,true);
}

void SetActiveTowingMode(boolean_t bActive)
{
    Trace("Towing Active : %d\n",bActive);
    m_stUserActionSetting.Towing.bActive = bActive;
}

#if defined(PROTOCOL12)
extern unsigned int Get_Odmeter(void);
eVEHICLE_STATE Get_VehicleStatus(void);
#endif

void SetUserActionSettingfromServer(stReportSetting rpSetting)
{
    switch(rpSetting.UserSetting.ucCommandType)
    {
        case eREMOTE_CON_CMD_TYPE_SENSOR_SENSITIVITY:
            SetGuardSensitivity(rpSetting.UserSetting.stUserActionSetting.Guard.stImpulseSetting);
            break;
        case eREMOTE_CON_CMD_TYPE_VALET:
            SetActiveValetMode(rpSetting.UserSetting.stUserActionSetting.Valet.bActive);
            break;
        case eREMOTE_CON_CMD_TYPE_TOWING:
            SetActiveTowingMode(rpSetting.UserSetting.stUserActionSetting.Towing.bActive);
            break;
        case eREMOTE_CON_CMD_TYPE_GUARD:
            SetActiveGuardMode(rpSetting.UserSetting.stUserActionSetting.Guard.bActive);
            break;
        case eREMOTE_CON_CMD_TYPE_DATA:
        	{
				stMsgMdm msg;
            // related with modem active is stored in internal flash
            // so we use internal flash write fucntion directly
            Trace("Data Active : %d\n",rpSetting.UserSetting.stUserActionSetting.ActiveModem.bActive);

            uint8_t bModemActive = rpSetting.UserSetting.stUserActionSetting.ActiveModem.bActive;

            SetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);

				msg.header.id = eMngSysMsg;
			    msg.header.event = eRspReport;
			    msg.header.subEvent = eR_RspSmartKey;
				msg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	            msg.header.drivingKey = Get_DrivingKey();

				msg.carReport.rpSmartKey.Response.ucSysSmartkeyReqType = eSysSmartkeyReqType_Modem;
				
				memcpy((char*)msg.carReport.rpSmartKey.Response.Guid,(char*)rpSetting.UserSetting.RequestGUID,MAX_GUID_LENGTH);
				
				msg.carReport.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
				msg.carReport.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();
				
				msg.carReport.rpSmartKey.Response.Result = eREMOTE_CON_RESULT_SUCCESS;
				msg.carReport.rpSmartKey.Response.Reason = 0;

				Send2MngModem2(&msg);
					}
            break;
        case eREMOTE_CON_CMD_TYPE_GEO_FENCE:
            SetGeofenceSetting(rpSetting.UserSetting.stUserActionSetting.Geofence);
            DisplayGeofenceSetting();
            break;
        case eREMOTE_CON_CMD_TYPE_POLYGON_GEO_FENCE:
            SetPolygonGeofenceSetting(rpSetting.UserSetting.stUserActionSetting.PolygonGeofence);
            break;
        case eREMOTE_CON_CMD_TYPE_SERVICE_TYPE:
            {
                Trace("Service Type : %x\n",rpSetting.UserSetting.stUserActionSetting.ServiceType.unServiceType);
                uint8_t unServiceType = rpSetting.UserSetting.stUserActionSetting.ServiceType.unServiceType;
                SetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&unServiceType);
            }
            break;

	    case eREMOTE_CON_CMD_TYPE_ENCRYPT_TYPE:
            {
                Trace("Encrypt Type : %x\n",rpSetting.UserSetting.stUserActionSetting.EncrryptType.unEncryptType);
                uint8_t unEncryptType = rpSetting.UserSetting.stUserActionSetting.EncrryptType.unEncryptType;
                SetAutolinkConfigProperty(eAutoLinkConfig_EncryptType,(void*)&unEncryptType);
	        }
            break;
#if defined(PROTOCOL12)
        case eREMOTE_CON_CMD_TYPE_MODEM_ACTIVE:
        	{
            	stMsgMdm msg;
            	memset((char*)&msg,0,sizeof(stMsgMdm));
            	// modem active precedure
            	// 1. check odometer
            	// 2. check power on
            	msg.header.id = eMngSysMsg;
            	msg.header.event = eRspReport;
           		msg.header.subEvent = eR_RspModemActivate;

#if defined(PROTOCOL18)
				if( IsDBCorrect() == true )
				{
#endif
            		// check vehicle status
	            	if( Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN ) //dh odo
	            	{
						if( Get_Odmeter() != 0 )
	               		{
	               			if( Get_Odmeter() == rpSetting.UserSetting.stUserActionSetting.ModemActivate.unOdometer )
	                		{

	                    		msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.ucResult = 1;
	                    		msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.usReason = 0;

	                    		uint8_t bModemActive = 1;
	                    		SetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
	                   		 }
	                 		 else
	                		{
	                    		msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.ucResult = 0;
	                    		msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.usReason = 2;// not match odometer
	                		}
	           		 	}
	           		 	else
	           		 	{
	           		 		msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.ucResult = 0;
	                    	msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.usReason = 8;// get odo delay
	           		 	}
	           		}
	            	else
	            	{
	               		 msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.ucResult = 0;
	               		 msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.usReason = 1;// power off error
	            	}
#if defined(PROTOCOL18)
				}
				else
				{
					msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.ucResult = 0;
	               	msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.usReason = 4; //DB Check error
				}
#endif
            	// 20200201 MONI
				// this code over write usersetting variable and this code didn't need for reporting message.
	            //memcpy((char*)msg.carReport.rpSmartKey.Response.Guid,(char*)rpSetting.UserSetting.RequestGUID,MAX_GUID_LENGTH);
	            //msg.carReport.rpSmartKey.Response.OccurredEventTime = GetLocalTime();
	            //msg.carReport.rpSmartKey.Response.OccurredEventUtcTime = GetUtcTimefromTime(GetLocalTime());
	            memcpy(msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.carrRequestGUID,(char*)rpSetting.UserSetting.RequestGUID,MAX_GUID_LENGTH);
	            msg.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.unEventTime = GetLocalTimefromTime(GetUTCTime());

	            Send2MngModem2(&msg);
        	}
            break;
#endif
#if defined(PROTOCOL15)
        case eREMOTE_CON_CMD_TYPE_SENSOR_INITIALIZE:
        {
            stSensorInfo stGyroAngle;
            stMsgMdm msg;
            memset((char*)&msg,0,sizeof(stMsgMdm));

            GetGyroNavieAngle(&stGyroAngle);
            stGyroAngle.bCalibration = true;
            SetGyroInitializeAngle(&stGyroAngle);
			g_bFuelLevelCheckFlag = true;
			UpdataGyroDefualtAngle();

            msg.header.id = eMngSysMsg;
            msg.header.event = eRspReport;
            msg.header.subEvent = eR_RspSensorInitialize;
            msg.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.ucResult = 1;
            msg.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.ucReason = 0;
            msg.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.sX = stGyroAngle.nX;
            msg.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.sY = stGyroAngle.nY;
            msg.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.sZ = stGyroAngle.nZ;

            Trace("Remote Control Sensor Initialize\n");
            Trace("X : %d\n",stGyroAngle.nX);
            Trace("Y : %d\n",stGyroAngle.nY);
            Trace("Z : %d\n",stGyroAngle.nZ);

            Send2MngModem2(&msg);
        }
            break;
#endif
#if defined(PROTOCOL18)
        case eREMOTE_CON_CMD_TYPE_SETTING_RSV_ENG_CTRL:
        	SetRsvEngCtrlSetting(rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl);
            break;
#endif

#if defined(FEATURE_EXTENSION_BOARD)
        case eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF:
        {
            stMsgExtend stExtBoardMsg;
            boolean_t bAntiThief;

            Trace("%s]eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF\n", __FUNCTION__);
            memset((char*)&stExtBoardMsg,0,sizeof(stExtBoardMsg));

            bAntiThief = rpSetting.UserSetting.stUserActionSetting.stAntiThiefCtl.bAntithiefFlag;

			SetAntiThiefFlagSetting(bAntiThief);

        }
            break;
#endif
            default:
            Trace("Error : UserActionSetting\n");
            break;
    }
}

void WriteDefaultUserAction()
{
    memset((char*)&m_stUserActionSetting,0,sizeof(m_stUserActionSetting));

    // low : 20 / middle : 50 / high : 70
    // set middle value
    m_stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel = 70;
    //m_stUserActionSetting.ActiveModem.bActive = true;
    //SetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&m_stUserActionSetting.ActiveModem.bActive);
}

boolean_t CheckGuard()
{
    stGuardSetting* pGuard = &m_stUserActionSetting.Guard;

    uint32_t unUTCTime = GetUTCTime();
    uint32_t unActiveTime = pGuard->unActiveTime;

#ifdef ENABLE_GUARD_LOG
    DisplayTime("Guard Active Time : ",unActiveTime);
    DisplayTime("Guard Check Time : ",unUTCTime);

    Trace("Guard] Active  Time : %d\n",unActiveTime);
    Trace("Guard] Current Time : %d\n",unUTCTime);
    Trace("Guard] Different Time : %d\n", unUTCTime - unActiveTime);
#endif //#ifdef ENABLE_GUARD_LOG

    if( (unUTCTime - unActiveTime) > pGuard->unKeepTime )
    {
        // keep time is expired
        Trace("Guard] time expired / initialize setting\n");
        pGuard->bActive = false;
        pGuard->unActiveTime = 0;
        pGuard->unKeepTime = 0;

        // save adjust setting
        WriteConfig(false, false);

        return true;
    }

    return false;
}

boolean_t GetGuardActive()
{
    return m_stUserActionSetting.Guard.bActive;
}


