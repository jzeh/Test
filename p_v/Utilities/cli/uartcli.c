#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "HalHandler.h"
#include "cli.h"
#include "app_cli.h"
#include "GIT_OemInterface.h"
#include "Modem_Manager.h"
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
#include "usbd_cdc_vcp.h"
#include "usbd_usr.h"
#endif
#include "UARTDMA_Manager.h"

//#define ENABLE_DEBUG_PASSWORD

#ifdef ENABLE_DEBUG_PASSWORD
	boolean_t m_bActiveCliManager = false;
#endif 

OLD_COMMANDS        commandBuffer;
COMMAND             commandInfo;
cli_init_data_t init_data;

extern uint8_t g_bEnableModemDirectCommunication;
extern uint8_t g_bEnableBLEDirectCommunication;

void PutChar(char ch);

void InitializeCommandLineInterface(void)
{
//  printf("\n\n");
//  printf("*********************************************************\n");
//  printf("Initializing the CLI facility and setting the CLI prompt.\n");
//  printf("*********************************************************\n");
  strcpy(init_data.prefix, "app");

  cli_init(&init_data);
  cli_set_prompt("GIT-AUTOLINK");

	/* make sure we can tolerate a null string */
	InitializeCommand();
	InitializeCommandBuffer();

	app_cli_init();

	cli_engine("");

	return;
}

void InitializeCommandBuffer(void)
{
  commandBuffer.newest    = MAX_BUFFERED_COMMANDS;
  commandBuffer.oldest    = MAX_BUFFERED_COMMANDS;
  commandBuffer.showing   = MAX_BUFFERED_COMMANDS;
}

/****************************************************************************
  Function:
    void InitializeCommand( void )

  Description:
    This function prints a command prompt and initializes the command line
    information.  If available, the command prompt format is:
                    [Volume label]:[Current directory]>

  Precondition:
    None

  Parameters:
    None

  Returns:
    None

  Remarks:
    None
  ***************************************************************************/

void InitializeCommand( void )
{
//	char buffer[50];

	commandInfo.index       = 0;
	commandInfo.reading     = TRUE;

	memset( commandInfo.buffer, 0x00, MAX_COMMAND_LENGTH );
}

/****************************************************************************
  Function:
    void EraseCommandLine( void )

  Description:
    This function erases the current command line.  It works by sending a
    backspace-space-backspace combination for each character on the line.

  Precondition:
    commandInfo.index must be valid.

  Parameters:
    None

  Returns:
    None

  Remarks:
    None
  ***************************************************************************/

void EraseCommandLine( void )
{
	uint8_t    i;
	uint8_t    lineLength;

	lineLength = commandInfo.index;
	for (i=0; i<lineLength; i++)
	{
		PutChar( 0x08 );
		PutChar( ' ' );
		PutChar( 0x08 );
	}
	commandInfo.index = 0;
}

/****************************************************************************
  Function:
    void ReplaceCommandLine( void )

  Description:
    This function is called when the user presses the arrow keys to scroll
    through previous commands.  The function erases the current command line
    and replaces it with the previous command indicated by
    commandBuffer.showing.

  Precondition:
    The buffer of old commands is valid.

  Parameters:
    None

  Returns:
    None

  Remarks:
    None
  ***************************************************************************/

void ReplaceCommandLine( void )
{
	uint8_t    i;
	uint8_t    lineLength;
	char    oneChar;

	EraseCommandLine();

	lineLength = strlen( commandBuffer.lines[commandBuffer.showing] );
	for (i=0; i<lineLength; i++)
	{
		oneChar = commandBuffer.lines[commandBuffer.showing][i];
		PutChar( oneChar );
		commandInfo.buffer[commandInfo.index++] = oneChar;
	}
}

/****************************************************************************
  Function:
    void UartSendToModem( void )

  Description:

  Precondition:
    None

  Parameters:
    None

  Returns:
    None

  Remarks:
    Currently, blank entries are added to the old command buffer.
  ***************************************************************************/
void UartSendToModem(uint8_t *pStr, uint16_t len)
{
    OemWriteUartModemBuff(pStr, len, NULL, eCOMM_TYPE_UART_MODEM);
}

void UartSendToBle(uint8_t *pStr, uint16_t len)
{
    OemWriteUartBTBuff(pStr, len, NULL, eCOMM_TYPE_UART_BT);
}


void CLI_Manager( void )
{
	char oneChar;
    uint8_t ret;
    signed short usReadLen = -1;

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	eHalFlagStatus bitstatus1 = HAL_RESET;
	eHalFlagStatus bitstatus2 = HAL_RESET;
    int16_t oneUSBCDCChar = -1;

	oneUSBCDCChar = USBCDCRxGetCh();

	if(oneUSBCDCChar != -1) {
		bitstatus2 = HAL_SET;
	}
	else {
		bitstatus2 = HAL_RESET;
	}
#endif

    if(g_bHYPERTECSelftestFlag == true)
#if defined(FEATURE_USE_UART_RX_DMA)
        if(Uart8Available() == true) 
        {
            oneChar = Uart8_DMARead();
            usReadLen = 1;
        }
        else
        {
            usReadLen = 0;
        }
#else
        usReadLen = UartReadBuf(&g_stUart8, (unsigned char*)&oneChar, 1);
#endif
    else
        usReadLen = UartReadBuf(&g_stUart7, (unsigned char*)&oneChar, 1);

    if ( usReadLen != 0 ) {

#ifdef ENABLE_DEBUG_PASSWORD
	if( m_bActiveCliManager == false )
	{
		if( oneChar == '\r' )
		{
		//hexdump(s_ucArrKey,s_ucArrIndex);
			if( memcmp(commandInfo.buffer,"fwdbgenable",11) == 0 )
			{
				m_bActiveCliManager = true;
			}
			memset(&commandInfo,0,sizeof(commandInfo));
		}
		else
		{
			if( commandInfo.index < 16 )
				commandInfo.buffer[commandInfo.index++] = oneChar;
		}
		return;
	}
#endif //#ifdef ENABLE_DEBUG_PASSWORD

    // If we are currently processing a command, throw the character away.
    if (commandInfo.reading) {
      if (commandInfo.escNeedSecondChar) {
        if (commandInfo.escFirstChar == 0x5B) {
          if (oneChar == 0x41) {        // Up arrow
//          	printf("UP\n");
            if (commandBuffer.showing != commandBuffer.oldest) {
              if (commandBuffer.showing == MAX_BUFFERED_COMMANDS) {
                commandBuffer.showing = commandBuffer.newest;
              }
              else {
								commandBuffer.showing = (commandBuffer.showing - 1) & (MAX_BUFFERED_COMMANDS-1);
							}
						}

						ReplaceCommandLine();
					}
					else if (oneChar == 0x42) {   // Down arrow
						printf("DOWN\n");
						if (commandBuffer.showing != MAX_BUFFERED_COMMANDS) {
							if (commandBuffer.showing != commandBuffer.newest) {
								commandBuffer.showing = (commandBuffer.showing + 1) & (MAX_BUFFERED_COMMANDS-1);
								ReplaceCommandLine();
							}
							else {
								EraseCommandLine();
								commandBuffer.showing = MAX_BUFFERED_COMMANDS;
							}
						}
						else {
							EraseCommandLine();
						}
					}
				}

				commandInfo.escNeedSecondChar   = FALSE;
			}
			else if (commandInfo.escNeedFirstChar) {
				commandInfo.escFirstChar        = oneChar;
				commandInfo.escNeedFirstChar    = FALSE;
				commandInfo.escNeedSecondChar   = TRUE;
			}
			else {
//				printf("ESC: %x\n", oneChar);
				if (oneChar == 0x1B) {    // ESC - an escape sequence
					commandInfo.escNeedFirstChar = TRUE;
				}
				else if (oneChar == 0x08) {    // Backspace
					if (commandInfo.index > 0) {
						commandInfo.index--;
						PutChar( 0x08 );
						PutChar( ' ' );
						PutChar( 0x08 );
					}
				}
				else if (oneChar == 0x7F) {    // Backspace
					if (commandInfo.index > 0) {
						commandInfo.index--;
						PutChar( 0x08 );
						PutChar( ' ' );
						PutChar( 0x08 );
					}
				}
				else if ((oneChar == 0x0D) || (oneChar == 0x0A)) {
					printf( "\r\n" );
					commandInfo.buffer[commandInfo.index]   = 0; // Null terminate the input command
					commandInfo.reading                     = FALSE;
					commandInfo.escNeedFirstChar            = FALSE;
					commandInfo.escNeedSecondChar           = FALSE;

					// Copy the new command into the command buffer
					commandBuffer.showing = MAX_BUFFERED_COMMANDS;
					if (commandBuffer.oldest == MAX_BUFFERED_COMMANDS) {
						commandBuffer.oldest = 0;
						commandBuffer.newest = 0;
					}
					else {
						commandBuffer.newest = (commandBuffer.newest + 1) & (MAX_BUFFERED_COMMANDS-1);
						if (commandBuffer.newest == commandBuffer.oldest)	{
							commandBuffer.oldest = (commandBuffer.oldest + 1) & (MAX_BUFFERED_COMMANDS-1);
						}
					}

					strcpy( &(commandBuffer.lines[commandBuffer.newest][0]), commandInfo.buffer );
				}
				else if ((0x20 <= oneChar) && (oneChar <= 0x7E)) {
//					oneChar = UpperCase( oneChar ); // To make later processing simpler
					if (commandInfo.index < MAX_COMMAND_LENGTH) {
						commandInfo.buffer[commandInfo.index++] = oneChar;
					}

					PutChar( oneChar );    // Echo the character
				}
			}
		}

		ret = GetCommand();
		if(ret == true) {
			if(g_bEnableModemDirectCommunication == true) {
				uint16_t n;

				if(commandInfo.buffer[0] == NULL) {
					InitializeCommand();

					return;
				}
				else if(memcmp(commandInfo.buffer, "CTRLZ", 5) == 0) {
					uint8_t aTemp[4];

					printf("\n<CTRL-Z>\n\n");

					aTemp[0] = 0x1A;

					UartSendToModem(aTemp, 1);

					InitializeCommand();

					return;
				}
				else if(memcmp(commandInfo.buffer, "exit", 4) == 0) {
					printf("\nexit modem direct interface!\n\n");
					g_bEnableModemDirectCommunication = false;

					HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);

					SetModemState(eMODEM_READY);

					InitializeCommand();

					return;
				}

				n = strlen(commandInfo.buffer);
				commandInfo.buffer[n] = 0x0D;

#if				0
				OemWriteUart2Buff((unsigned char *)commandInfo.buffer, n + 1, NULL, NULL);
#else
				UartSendToModem((uint8_t *)commandInfo.buffer, n + 1);
#endif
			}
			else if(g_bEnableBLEDirectCommunication == true) {
				uint16_t n;

				if(commandInfo.buffer[0] == NULL) {
					InitializeCommand();

					return;
				}
				else if(memcmp(commandInfo.buffer, "exit", 4) == 0) {
					printf("\nexit BLE direct interface!\n\n");
					g_bEnableBLEDirectCommunication = false;

					InitializeCommand();

					return;
				}

				n = strlen(commandInfo.buffer);
				commandInfo.buffer[n] = 0x0D;
#if             0
				OemWriteUart3Buff((unsigned char *)commandInfo.buffer, n + 1, NULL, NULL);
#else
				UartSendToBle((unsigned char *)commandInfo.buffer, n + 1);
#endif
			}
			else {
				cli_engine(commandInfo.buffer);
			}

			InitializeCommand();
		}
	}

	return;
}

/****************************************************************************
  Function:
    uint8_t GetCommand( void )

  Description:
    This function returns whether or not the user has finished entering a
    command.  If so, then the command entered by the user is determined and
    placed in commandInfo.command.  The command line index
    (commandInfo.index) is set to the first non-space character after the
    command.

  Precondition:
    commandInfo.reading must be valid.

  Parameters:
    None

  Return Values:
    TRUE    - The user has entered a command.  The command is in
                  commandInfo.command.
    FALSE   - The user has not finished entering a command.

  Remarks:
    None
  ***************************************************************************/

uint8_t GetCommand( void )
{
	if (commandInfo.reading) {
		return FALSE;
	}
	else {
		commandInfo.index = 0;

		return TRUE;
	}
}

void PutChar(char ch)
{
    GITDebugPrintf(&ch);

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	if(gbUSBCDCConnected == true && gbEnableUSBCDC == true) {
		VCP_DataTxByte(ch);
	}
#endif
}
