/*----------------------------------------------------------------------
 *   record control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_RECORD_CONFIG_H__
#define	__GIT_RECORD_CONFIG_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "typedef.h"

#include "git_PassthruDefines.h"
//MONI #include "PassThruStruct.h"

#include "git_record.h"
#include "git_trigger.h"


//#define ENABLE_TEST_CONFIG_BUFFER



bool ReadEcuStallConfig(uint8_t ucSystemPos, tagEngineStopTiggerInfo* pstEngStopTrgInfo);
bool ReadVehicleInfoFromConfig(stVehicleInfo* pstVehicleInfo);
bool ReadEcuSystemInfoFromConfig(uint8_t ucSystemPos, stConfigControl* pstSystemConfig);



#endif // __GIT_RECROD_CONFIG_H__
