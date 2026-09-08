/* Includes ------------------------------------------------------------------*/
#include "AutolinkConfig.h"
#include "Message_Manager.h"

#include "GIT_Util.h"

#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"

boolean_t m_bForwardingSmartKey2Bt = false;
boolean_t GetForwardingSmartKey2BtFlag();
void SetForwardingSmartKey2BtFlag(boolean_t bForwardingSmartKey2Bt);
void ResponseSmartKey2Bt(stMsgSysMsg* pstMsgSysMsg);
uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern uint32_t GetUTCTime(); //mod.kks 21.11.05 to modify the warnning.

void SetForwardingSmartKey2BtFlag(boolean_t bForwardingSmartKey2Bt)
{
    m_bForwardingSmartKey2Bt = bForwardingSmartKey2Bt;
}

boolean_t GetForwardingSmartKey2BtFlag()
{
    return m_bForwardingSmartKey2Bt;
}

int32_t EHL_MngBt(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.event == eReqForwardingSmartkey2Bt )
    {
        if( pstMsgSysMsg->header.subEvent == eTrue )
        {
            SetForwardingSmartKey2BtFlag(true);
        }
        else
        {
            SetForwardingSmartKey2BtFlag(false);
        }
    }
    else if( pstMsgSysMsg->header.event == eReqReport )
    {
    	//if( m_stMsgHdData.bAlreadyRcvSmartkey == false )
    	{    	
	        // this code will be treated by bluetooth
	        Send2MngObd(eMngSysMsg, eReqReport, eR_ReqSmartKey ,(stCarReport*)&pstMsgSysMsg->carReport, pstMsgSysMsg->header.unTraceMng);
        }
        //else
        //{
        	// response fail to bluetooth
        	
        //}
    }
    //else if( pstMsgSysMsg->header.event == eRspReport )
    //{
    //    // this code will be sent to bluetooth
    //}
    else
    {
        // throw away the other events
    }
    return 0;
}

// test reqeust an event for forwarding smartkey
void RequestForwardingSmartKey(boolean_t bReqForwardingSmartKey)
{
    if( bReqForwardingSmartKey == true )
        Send2MngSysMsg(eMngBt, eReqForwardingSmartkey2Bt, eTrue, (stCarReport *)NULL,0);
    else
        Send2MngSysMsg(eMngBt, eReqForwardingSmartkey2Bt, eFalse, (stCarReport *)NULL,0);
}

stMsgSysMsg m_stBtLastRequestSmartKeyMessage;
boolean_t m_bBtLastRequestSmartKeyMessage = false;

void SetBtLastRequestSmartkey(stMsgSysMsg* pstMsgSysMsg)
{
    m_bBtLastRequestSmartKeyMessage = true;
    memcpy((char*)&m_stBtLastRequestSmartKeyMessage,(char*)pstMsgSysMsg,sizeof(stMsgSysMsg));
}

boolean_t GetBTLastRequestSmartkey(stMsgSysMsg* pstMsgSysMsg)
{
    if( m_bBtLastRequestSmartKeyMessage == true )
    {
        memcpy((char*)pstMsgSysMsg,(char*)&m_stBtLastRequestSmartKeyMessage,sizeof(stMsgSysMsg));
        m_bBtLastRequestSmartKeyMessage = false;
        return true;
    }

    return false;
}

void RequestSmartKey2System( eREMOTE_CON_CMD_TYPE eReqCommand,
                            uint8_t ucControlType,
                            uint8_t ucDefrost,
                            uint8_t ucKeepPowerOnTime,
                            uint16_t usTemperature,
                            uint8_t* pucGuid )
{
    stMsgSysMsg stReport;

    memset((char*)&stReport,0,sizeof(stCarReport));

    stReport.carReport.rpSmartKey.Request.CommandType = eReqCommand;
    stReport.carReport.rpSmartKey.Request.ControlType = ucControlType;
    stReport.carReport.rpSmartKey.Request.Defrost = ucDefrost;
    memcpy(stReport.carReport.rpSmartKey.Request.Guid,pucGuid,MAX_GUID_LENGTH);
    stReport.carReport.rpSmartKey.Request.KeepPowerOnTime = ucKeepPowerOnTime;
    stReport.carReport.rpSmartKey.Request.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    stReport.carReport.rpSmartKey.Request.Temperature = usTemperature;

    Send2MngSysMsg3(&stReport);

    SetBtLastRequestSmartkey(&stReport);
}

void ResponseSmartKey2Bt(stMsgSysMsg* pstMsgSysMsg)
{
    // handle this response in bt
    stMsgSysMsg stMessage;

    if( GetBTLastRequestSmartkey(&stMessage) == true )
    {
        if( memcmp(stMessage.carReport.rpSmartKey.Request.Guid,
            pstMsgSysMsg->carReport.rpSmartKey.Response.Guid,
            MAX_GUID_LENGTH) == 0 )
        {
            // this is request command
            // response to bt
            // example
            switch(stMessage.carReport.rpSmartKey.Request.CommandType)
            {
                case eREMOTE_CON_CMD_TYPE_STARTING:
                    // response to bt according to the command
                    break;
	            case eREMOTE_CON_CMD_TYPE_DOOR:
                    // response to bt according to the command
                    break;
	            case eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT:
                    // response to bt according to the command
                    break;
                case eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT:
                    // response to bt according to the command
                    break;
                default :
                    // error  // not defined
                    break;
            }

        }
        else
        {
            // not match request
        }
    }
}


void TestRequestDoorOpen()
{
    char guid[MAX_GUID_LENGTH]={0,};
    RequestSmartKey2System(eREMOTE_CON_CMD_TYPE_DOOR,1,0,0,0,(uint8_t *)guid);
}

void TestRequestPoweron()
{
    char guid[MAX_GUID_LENGTH]={1,};
    RequestSmartKey2System(eREMOTE_CON_CMD_TYPE_STARTING,1,1,10,0x9004,(uint8_t *)guid);
}

