/*----------------------------------------------------------------------
 *   FDCAN Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_LISTDIAG_H__
#define	__GIT_LISTDIAG_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define MESSAGE_LISTDIAG_QUEUE_SIZE							50
typedef enum {
	LISTDIAG_NONE = 0xFE00,
	LISTDIAG_INIT = 0xFE01,
    LISTDIAG_RUNNING = 0xFE02,
    LISTDIAG_CHECK = 0xFE03,
    LISTDIAG_FAIL = 0xFE04,
    LISTDIAG_END = 0xFE05,
    LISTDIAG_MAX
} LISTDIAG_STATUS;
typedef enum {
	LISTSENSOR_NONE = 0xFE10,
	LISTSENSOR_INIT = 0xFE11,
    LISTSENSOR_RUNNING = 0xFE12,
    LISTSENSOR_CHECK = 0xFE13,
    LISTSENSOR_FAIL = 0xFE14,
    LISTSENSOR_END = 0xFE15,
    LISTSENSOR_MAX
} LISTSENSOR_STATUS;
#define LISTDIAG_START		0x6000
#define LISTDIAG_RES		0x1002
#define LISTDIAG_END_RES	0x1002
#define LISTSENSOR_START	0x6005
#define LISTSENSOR_RES		0x1002
#define LISTSENSOR_END_RES	0x1002
#define RESDATA_ADDINFOSIZE 0x3
#define MAX_REQ_COUNT		2
#define LISTDIAGSIZE		2600
#define LISTDIAGSTRUCTMAX	100
#define LISTDIAGENDSTR		"LISTDIAGEND"
#define LISTSENSORENDSTR	"LISTSENSOREND"
#define PASSTHRUELENGTHPOS	16
#define RETRYCOUNT			3    //must define over 1
/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
void ListDiagThread( void const *argument );
void ListDiagtask(LISTDIAG_STATUS eInputStatus);
uint8_t ListDiagParsing(uint8_t *ucBuff,uint16_t usLength);
void ListDiagRunProcess(LISTDIAG_STATUS eInputStatus);
void InitGITSetConfig();
void InitGITHWSetData();
void SendMSGToListDiag(u16 Mode, stMsgClst *message );
void MSGChangeToListDiag(stCommPkt *packet );
bool StartlistdiagThread();
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
#endif // __GIT_LISTDIAG_H__