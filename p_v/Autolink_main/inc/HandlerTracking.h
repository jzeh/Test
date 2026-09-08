/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HANDLER_TRACKING_CTRL_H__
#define __HANDLER_TRACKING_CTRL_H__

#include "AutolinkMessage.h"
#include "AutolinkConfiguration.h"

typedef enum __eTrackingReasonofResult{
    eTM_REASON_NONE						   = 0x00000000,
    eTM_REASON_ACTIVE_OPERATING        	   = 0x00000001,
    eTM_REASON_ACTIVE_NOTRANGE        	   = 0x00000010,
    eTM_REASON_DURATION_NOTRANGE       	   = 0x00000100,
    eTM_REASON_INTERVAL_NOTRANGE		   = 0x00001000,   
}eTrackingReasonofResult;

typedef enum __eTrackingActive{
    eTM_ACTIVE_OFF = 0,
    eTM_ACTIVE_ON,
}eTrackingActive;




void GetTrackingSetting(stTrackingControl* pstTrackingValue);
void SetTrackingSetting(stTrackingControl* pstTrackingValue);
void DispTrackingSetting();
boolean_t GetTrackingActive();
void CheckTracking();
void GetTrackingActiveGuid(uint8_t* pcArrGuid);

#endif // __HANDLER_TRACKING_CTRL_H__

/***************************** END OF FILE ****/

