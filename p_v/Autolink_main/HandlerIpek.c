/* Includes ------------------------------------------------------------------*/
#include "AutolinkMessage.h"
#
#include "HdDebug.h"
#include "Message_make.h"

#include <math.h>

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

extern boolean_t GetBlockMsgTrasfer();
extern int GetPublickey(char* parrPublicKey,int* pnPublicKeySize);

stReportIpek IpekProperties;
unsigned char m_ucRandValue[16]={0,};

void InitIpekProperties()
{
    memset((char*)&IpekProperties,0,sizeof(stReportIpek));
}

void SetIpekProperties(stReportIpek* pstIpek)
{
    //memset((char*)&IpekProperties,0,sizeof(stReportIpek));
    memcpy((char*)&IpekProperties,(char*)pstIpek,sizeof(stReportIpek));
}

void GetIpekProperties(stReportIpek* pstIpek)
{
    memcpy((char*)pstIpek,(char*)&IpekProperties,sizeof(stReportIpek));
}

typedef struct __stIpekList{
    char ipek[16];
}stIpekList;


extern char* m_strTestSerialList[];
extern stIpekList m_stTestIpekList[];

#ifdef TEST_IPEK
int m_ui=3;
#endif

//extern const unsigned char g_KMS_Publickey[];

void HandlerIpek(stMsgSysMsg* pstMsgSysMsg)
{
    stReportIpek stIpekMessage;
    memset((char*)&stIpekMessage,0,sizeof(stReportIpek));

    char key[1024]={0,};
    int nKeySize = 0;

    GetPublickey(key,&nKeySize);
    
    
    if( pstMsgSysMsg->header.event == eReqIpek )
    {
        Send2MngModem(eMngSysMsg,eReqIpek,eIpekPhase1,(stCarReport *)NULL,0);
    }
    else if( pstMsgSysMsg->header.event == eRspIpek )
    {
        
        unsigned char ipekKey[16];
        unsigned char ucPublicEnc[512];

        GetIpekProperties(&stIpekMessage);

        if( pstMsgSysMsg->header.result == eFalse )
        {
            Trace("IPEK Process Result Error\n");
            return;
        }
        
        if( pstMsgSysMsg->header.subEvent == eIpekPhase1 )
        {            
            Trace("srand : \n");hexdump(stIpekMessage.Phase1.Srand,stIpekMessage.Phase1.SrandSize);
            Trace("hash : \n");hexdump(stIpekMessage.Phase1.Hash,stIpekMessage.Phase1.HashSize);
            Trace("sign : \n");hexdump(stIpekMessage.Phase1.Sign,stIpekMessage.Phase1.SignSize);
            
            // rcv properties for making ksn and rand 
            // must response with that values to server
            if(!DAMO_DUKPT_Client_PK_Auth_Start((unsigned char*)key,
                stIpekMessage.Phase1.Srand,
                stIpekMessage.Phase1.Hash,
                stIpekMessage.Phase1.Sign,
                m_ucRandValue,
                ucPublicEnc))
            {
                unsigned char ucOpenSerial[SIZE_SERIAL_NUMBER+1];
				memset(&ucOpenSerial,0x00,sizeof(ucOpenSerial));
#ifdef TEST_IPEK
                if( m_ui == 12 )
                    m_ui = 3;
#endif //#ifdef TEST_IPEK

                Trace("success ipek phase 1\n");
                // get serial number for ksn
                
#ifdef TEST_IPEK
                memcpy(&ucOpenSerial[0],m_strTestSerialList[m_ui++], SIZE_SERIAL_NUMBER);                
#else //#ifdef TEST_IPEK
                memcpy(&ucOpenSerial[0],GetFWSerialNumber(), SIZE_SERIAL_NUMBER);
#endif //#ifdef TEST_IPEK

                Trace("Serial : %s\n",ucOpenSerial);    

                memset(stIpekMessage.Phase2.Ksn,'0',sizeof(stIpekMessage.Phase2.Ksn));
                memcpy(stIpekMessage.Phase2.Ksn,ucOpenSerial,8);
                Trace("ksn :\n");hexdump(stIpekMessage.Phase2.Ksn,11);
                stIpekMessage.Phase2.KsnSize = 11;
                
                stIpekMessage.Phase2.EncRandSize = 256;
                memcpy(stIpekMessage.Phase2.Enc,ucPublicEnc,512);

                //ConversionIntToHexDec_2(stIpekMessage.Phase2.Ksn, 0, &buffer);

                Trace("ksn : \n");hexdump(stIpekMessage.Phase2.Ksn,stIpekMessage.Phase2.KsnSize);
                Trace("enc : \n");hexdump(stIpekMessage.Phase2.Enc,512);

                SetIpekProperties(&stIpekMessage);
                // request ipek phase 2
                Send2MngModem(eMngSysMsg,eReqIpek,eIpekPhase2,(stCarReport *)NULL,0);
            }
            else
            {
                Trace("========================================\r\n");
                Trace("Error IPEK Phase1\r\n");
            }            
        }
        else if( pstMsgSysMsg->header.subEvent == eIpekPhase2 )
        {
            Trace("ipek : \r\n");hexdump(stIpekMessage.Ipek.Ipek,stIpekMessage.Ipek.IpekSize);
            Trace("ksn : \r\n");hexdump(stIpekMessage.Phase2.Ksn,stIpekMessage.Phase2.KsnSize);      
            
            // received ipek , set up ipek and future key to system
            if(!DAMO_DUKPT_Client_PK_Auth_End_Ex((unsigned char*)key, 
                stIpekMessage.Ipek.Ipek, 
                stIpekMessage.Phase2.Ksn, 
                sizeof(stIpekMessage.Phase2.Ksn), 
                m_ucRandValue, 
                ipekKey) )
            {
                Trace("success ipek phase 2\r\n");
                // set ipek to system                

                // apply new ipek from server 
                SetSerialAndIpektoInternaFlash((char*)ipekKey,16);

#ifdef TEST_IPEK
                Trace("=======================================================\r\n");
                Trace("=======================================================\r\n");
                Trace("=======================================================\r\n");

                if( memcmp(&m_stTestIpekList[m_ui-1].ipek[0],ipekKey,16) == 0 )
                {
                    Trace("IPEK GET Success\r\n");
                    Trace("IPEK : \r\n");hexdump(ipekKey, 16);    
                }
                else
                {
                    Trace("IPEK GET Fail\r\n");
                    Trace("Origianl :\r\n");hexdump(ipekKey, 16);    
                    Trace("Received :\r\n");hexdump(&m_stTestIpekList[m_ui].ipek[0], 16);    

                    GIT_Assert(false,eErrorCodeApp|eIpekGetFail);
                }
                Send2MngSysMsg(eMngSys,eReqIpek,eIpekPhase1,(stCarReport *)NULL,0);
#endif //#ifdef TEST_IPEK                
            }
            else
            {
                Trace("========================================\r\n");
                Trace("Error IPEK Phase2\r\n");
            }
        }
    }    
}

