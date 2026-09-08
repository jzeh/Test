
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __OBD_CGW_SECUTRY_H__
#define __OBD_CGW_SECUTRY_H__

#include "OBD_Manager.h"
#include "GIT_OemInterface.h"
#include "CanFD_Controller.h"
#include "GIT_CanParsingProc.h"

#define CGW_TIMEOUT_TIME	(20000)

void OBDCGWAlgorithm();
bool CGW_Security_Algorithm(unsigned char*,unsigned char*);

enum{
    eCGW_None = 0,
	eCGW_Dummy,
	eCGW_CarbOpen,
	eCGW_ExtOpen,
	eCGW_Security1,
	eCGW_Security2,
	eCGW_SecurityDone,
    eCGW_MAX
};


#endif /* __OBD_CGW_SECUTRY_H__ */

/***************************** END OF FILE ****/
