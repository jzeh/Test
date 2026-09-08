/*----------------------------------------------------------------------
 *   FDCAN Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_CSACDIAG_H__
#define	__GIT_CSACDIAG_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define MESSAGE_CSACDIAG_QUEUE_SIZE							20
typedef enum {
	CSACDIAG_NONE = 0xFE00,
	CSACDIAG_INIT = 0xFE01,
    CSACDIAG_RUNNING = 0xFE02,
    CSACDIAG_CHECK = 0xFE03,
    CSACDIAG_FAIL = 0xFE04,
    CSACDIAG_END = 0xFE05,
    CSACDIAG_MAX
} CSACDIAG_STATUS;

#define CSACDIAGSIZE    1300
#define CSACDIAGSTRUCTMAX    2
#define PASSTHRUELENGTHPOS 16

#define SECURE_ACCESS_STEP    1
#define SECURE_ACCESS_END     2
#define SECURE_ACCESS_ERROR   3

#define CSAC10          0x1212
#define CSAC20          0x1211

#define SUCCESS               0
#define FAIL                  1
/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
void CsacDiagThread( void const *argument );
void CsacDiagtask(CSACDIAG_STATUS eInputStatus);
void CsacDiagRunProcess(CSACDIAG_STATUS eInputStatus);
void InitGITSetConfig2();
void InitGITHWSetData2();
void SendMSGToCsacDiag(u16 Mode, stMsgClst *message );
bool StartCsacdiagThread();
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/

#endif // __GIT_LISTDIAG_H__