ƒ//#include <p32xxxx.h>
//#include <string.h>
#include <ctype.h>
#include <stdlib.h>
//#include <peripheral/uart.h>
#include <plib.h>
#include <time.h>
#include "HardwareProfile.h"
#include "defines.h"
#include "delay.h"
#include "main.h"
#include "memorymap.h"
#include "timer.h"
#include "utils_asciihex.h"
#include "gsm.h"
#include "atc_pwm.h"
#include "delay.h"
#include "uarts.h"
#include "spis.h"
#include "SST25.h"
#include "SST25VF064C.h"
//#include "atc_control.h"
#include "atc_time.h"
#include "atc_io.h"
#include "atc_servercom.h"

#include <stdint.h>

//DY
#include "comm.h"
#include "server_comm.h"
#include "tracer.h"
#include "utils.h"
#include "crc_hw.h"

const char* gsm_responses[][2] = {
		{"0\r", "OK\r\n"}, //0
		{"3\r", "NO CARRIER"} //1
};


//char AtCind[18];
//char IMSI[16] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
//char IMEI[16] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
//memset( IMSI, 0, 16);	// Null at end

void gsmSendEscapeSequence(void)
{
    DelayMs(1500);
    writeByteToUxRTx( UART_GSM, '+' );
    writeByteToUxRTx( UART_GSM, '+' );
    writeByteToUxRTx( UART_GSM, '+' );
    DelayMs(1500);
}

/* Does not funtion */
int gsmSuspendSocket( int maxTries )
{
	int i;
	int nError = 0;

//CONNECT
//
//+++
//
//
//
//OK
//
//aatt##sshh==11
//
//
//
//OK
//
//
	for( i = 0; i < maxTries; i++ )
	{
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nSetting Module in command mode\r\n" );
#endif
//		DelayMs( 1500 );
//		writeStringToUxRTx( UART_GSM, "+++" );
//		//writeByteToUxRTx( UART_GSM, '+' );
//		//writeByteToUxRTx( UART_GSM, '+' );
//		DelayMs( 1500 );
		gsmSendEscapeSequence();

#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
		if( waitForString( "0\r", delay3s ) )
#else
		if( waitForString( "OK\r\n", delay3s ) )
#endif
		{
			nError = 1;
			break;
		}
		//gsmCloseSocket( 4, 1);

		/* Test if we have successfully changed to command mode on module */
//		if( gsmIsOn() )
//		{
//			nError = 1;
//			break;
//		}
	}

	return nError;
}
/* Alias done i gsm.h                                                                 */
//int gsmSetCommandMode( int maxTries ) __attribute__ ((alias("gsmSuspendSocket")));


int gsmResumeSocketConnection( int maxTries, char socketNr )
{
	int i;
	int nError = 0;

	socketNr += 0x30;

	for( i = 0; i < maxTries; i++ )
	{
		if( gsmSendAtCommand( "#SO=" ) )
		{
			writeByteToUxRTx( UART_GSM, socketNr );
			writeByteToUxRTx( UART_GSM,'\r' );
			nError = 1;
			break;
		}	
	}

	return nError;
}

// HB changed 20111223
// TODO - Test for stability and function
int gsmCloseSocket( int maxTries, char socketNr)
{
	int i;
	int nError = 0;

	socketNr += 0x30;

	// Close Socket <nr>
	for( i = 0; i < maxTries; i++ )
	{
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nClosing socket 1, AT#SH=" );
		writeByteToUxRTx( UART_DEBUG, socketNr );
		writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif
		if( gsmSendAtCommand( "#SH=" ) )
		{
			writeByteToUxRTx( UART_GSM, socketNr );
			writeByteToUxRTx( UART_GSM,'\r' );
			// Wait for "OK"
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
			if( waitForString( "0\r", delay1s ) )
#else
			if( waitForString( "OK\r\n", delay1s ) )
#endif
			{
				nError = 1;
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "Socket 1 closed\r\n" );
#endif
				break;
			}
			else
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "ERROR: closing socket 1\r\n" );
#else
				;
#endif
		}
	}

	return nError;
}


/******************************************************************************
* Parameter:	none
*
* Return value:	0 on error,
*				else on ok.
*
* Abbreviations:	Module -> GSM module
*
* Recalculate received virtual address from Server to physical address in Node
* and return what memory partition we are working with.
******************************************************************************/
int gsm_init(unsigned int* gsmStatus, char* IMEI, char* IMSI, char* AtCind)
{
  int i;

#ifdef __UART_DEBUG_gsmInit
  writeStringToUxRTx( UART_DEBUG, "\r\nStarting initiation of module\r\n");
#endif
  /* Clear UART buffer from old data */
  clrUARTRxBuffer( UART_GSM);

  /* Try to establish first contact with module */
#ifdef __UART_DEBUG_gsmInit
  writeStringToUxRTx( UART_DEBUG, "Turning module on\r\n");
#endif
  if (!gsmTurnOn(10)) {
#ifdef __UART_DEBUG_gsmInit
    writeStringToUxRTx( UART_DEBUG, "Could not turn on module\r\n");
#endif
    *gsmStatus &= ~GSM_ON;
    return 0;
  } else {
    if (gsmCheckIfAnsweringOnATCommand()) {
      *gsmStatus |= GSM_ON;
    } else {
      *gsmStatus &= ~GSM_ON;
      return 0;
    }
  }

  gsmResetModuleToFactoryDefault();

  for (i = 0; i < 4; i = i + 1) {
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
    if (writeStringToUxRTx( UART_GSM, "ATV0\r")) {  // <numeric code><CR>.
      if (waitForString("0\r", delay3s)) {
#ifdef __UART_DEBUG
        writeStringToUxRTx( UART_DEBUG,
            "Response format set to '<numeric code><CR>'\r\n");
#endif
        break;
      }
    }
#else
    if (writeStringToUxRTx( UART_GSM, "ATV1\r")) { // <numeric code><CR>.
      if (waitForString("OK\r", delay3s))
      {
        writeStringToUxRTx( UART_DEBUG, "Response format set to '<text><CR><LF>'\r\n" );
        break;
      }
    }
#endif
  }
  if (i > 3);
//            error

  setCommandEchoMode(0, 10);
  // 0 - disables command echo
  // 1 - enables command echo (factory default) , hence command sent to the device
  // are echoed back to the DTE before the response is given.

  gsmEnableResultCodeLevel("0", 5);
  // 0 - enables result codes (factory default)
  // 1 - every result code is replaced with a <CR>
  // 2 - disables result codes

  // Set fixed Baudrate
//	if( gsmSetIPR( "115200" ) )
  if (gsmSetIPR("9600")) {
    *gsmStatus |= GSM_IPR_SET;
  } else {
    *gsmStatus &= ~GSM_IPR_SET;
    return 0;
  }

	// Set flowcontol
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\n\nSetting flow control\r\n" );
#endif
	if( gsmSetFlowControl() )
	{
		*gsmStatus |= GSM_FLOW_CONTROL_SET;
	}
	else
	{
		*gsmStatus &= ~GSM_FLOW_CONTROL_SET;
		return 0;
	}

	// AT#SELINT=2
	// Use always the newer AT Commands Interface Style.
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nSetting command level 2\r\n" );
#endif
	if( gsmSetCommandLevel() )
	{
		*gsmStatus |= GSM_COMMAND_LEVEL_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_COMMAND_LEVEL_SET );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_COMMAND_LEVEL_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
		return 0;
	}

	// AT+CMEE=2
	// Enable +CME ERROR: <err> reports, with <err> in verbose format.
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nSetting device error format to \"ERROR\"\r\n" );
#endif
	if( gsmSetErrorLevel() )
	{
		*gsmStatus |= GSM_ERROR_LEVEL_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_ERROR_LEVEL_SET );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_ERROR_LEVEL_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
		return 0;
	}

	// The GSM module will not update its time from the GSM network.
	for(i = 0; i < 5; i++)
	{
		int status = 0;

		status = gsmUpdateTimeFromGsmNetwork('0');

#ifdef __UART_DEBUG_gsmInit
		if(status==0)
			writeStringToUxRTx( UART_DEBUG, "\r\nDisable update time zone.\"\r\n" );
		if(status==1)
			writeStringToUxRTx( UART_DEBUG, "\r\nDisable automatic update time zone.\"\r\n" );	// Automatic Time Zone update - +CTZU.
		if(status==2)
			writeStringToUxRTx( UART_DEBUG, "\r\nDisable update by network time zone.\"\r\n" );	// Network Timezone - #NITZ.
		if(status==3)
			writeStringToUxRTx( UART_DEBUG, "\r\nDisable both automatic update time zone and by network time zone.\"\r\n" );	// +CTZU and #NITZ.
#endif

		if(status)
			break;
	}

	// Check GSM module's IMEI.
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nCheck module's IMEI\r\n" );
#endif
	gsm_get_imei( IMEI );
	if( IMEI[12] )  // All alpanum in IMEI, no null.
	{
		*gsmStatus |= GSM_IMEI;
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, IMEI );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_IMEI;
		return 0;
	}

	// Check SIM cards IMSI
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nCheck SIM cards IMSI\r\n" );
#endif
	//DelayMs( 1000 );
	gsm_get_imsi( IMSI );
	//if( IMSI[0] )
	if( IMSI[12] )  // All alpanum in IMSI, no null.
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSIM card's IMSI\r\n" );
		//nError = 1;
		writeStringToUxRTx( UART_DEBUG, "IMSI = " );
		for( i = 0; i < 15; i++)
			writeByteToUxRTx( UART_DEBUG, IMSI[i] );
		writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif
		*gsmStatus |= GSM_IMSI;
	}
	else
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSIM card's ISMI NOT received\r\n" );
#endif
		*gsmStatus &= ~GSM_IMSI;
	}

	/* Check if SIM is inserted */
//	writeStringToUxRTx( UART_DEBUG, "\r\nChecking if SIM card is inserted\r\n" );
	/*UARTSendDataByte( UART_GSM, 'A' );
	UARTSendDataByte( UART_GSM, 'T' );
	writeStringToGSM( "+CIMI\r" );
	if( waitForString( "OK\r\n") );*/
		/* Do something with this - set a flag */
/*	if( gsmCheckSimCardInserted( IMSI ) )
	{
		*gsmStatus |= GSM_SIM_CARD_INSERTED;
		writeInt( GSM_SIM_CARD_INSERTED );
	}	
	else
	{
		*gsmStatus &= ~GSM_SIM_CARD_INSERTED;
		writeInt( 0 );
	}	
*/
	// AT+CSCS="IRA" -> international reference alphabet (ITU-T T.50)
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nSetting alphabet to IRA (ITU-T /.50)\r\n" );
#endif
	if( gsmSetTeCharacterSet() )
	{
		*gsmStatus |= GSM_TE_CHARACTER_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_TE_CHARACTER_SET );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_TE_CHARACTER_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
		return 0;
	}

	// AT#TCPREASS=1 -> Enable TCP Reassembly
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nEnabling TCP reassembling in module\r\n" );
#endif
	if( gsmSetTcpReassembly() )
	{
		*gsmStatus |= GSM_TCP_REASSEMBLY_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_TCP_REASSEMBLY_SET );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_TCP_REASSEMBLY_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
		return 0;
	}

	// AT#USERID=""	-> No userid
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nClearing user id\r\n" );
#endif
	if( gsmSetUserId() )
	{
		*gsmStatus |= GSM_USER_ID_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_USER_ID_SET );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_USER_ID_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
		return 0;
	}

	// AT#PASSW="" -> No password
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nClearing user password\r\n" );
#endif
	if( gsmSetUserPassword() )
	{
		*gsmStatus |= GSM_USER_PASSWORD_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_USER_PASSWORD_SET );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_USER_PASSWORD_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
	}

	// AT#ICMP=1 -> Allow ping in firewall rules
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nSetting ICMP permission to firewall rules\r\n" );
#endif
	// '0' - No ICMP message allowed.
	// '1' - PING message allowed if rule in firewall.
	if( gsmSetIcmp( '1' ) )
	{
		*gsmStatus |= GSM_ICMP_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_ICMP_SET );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_ICMP_SET;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
		return 0;
	}

	// Store i profile 0
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nStore settings in profile 0\r\n" );
#endif
	if( gsmStoreInProfile0() )
	{
		*gsmStatus |= GSM_STORED_IN_PROFILE_0;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_STORED_IN_PROFILE_0 );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_STORED_IN_PROFILE_0;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
	}

	// Set initial firewall rules
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nInitiating Firewall\r\n" );
#endif
	if( gsmSetFirewallInit() )
	{
		*gsmStatus |= GSM_FIREWALL_INITIATED;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_FIREWALL_INITIATED );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_FIREWALL_INITIATED;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
	}

	// Initiate GPRS
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nInitiate GPRS\r\n" );
#endif
	if( gsmInitGprs( "1,\"IP\",\"public.telenor.se\",\"0.0.0.0\",0,0" ) )
	{
		*gsmStatus |= GSM_GPRS_INITIATED;
#ifdef __UART_DEBUG_gsmInit
		writeInt( GSM_GPRS_INITIATED );
#endif
	}
	else
	{
		*gsmStatus &= ~GSM_GPRS_INITIATED;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
//		return 0;
	}

	/* Configure for SMTP mail sending */
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nConfigure e-mail\r\n" );
#endif
	if( !gsmSetEmailConfig() )
//		return 0;
		;

	/* Check if GSM module initiated */
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nCheck if module initiated\r\n" );
#endif
	if( *gsmStatus & (	GSM_ON | GSM_IPR_SET | GSM_FLOW_CONTROL_SET | GSM_COMMAND_LEVEL_SET |
						GSM_ERROR_LEVEL_SET | GSM_SIM_CARD_INSERTED | GSM_TE_CHARACTER_SET |
						GSM_TCP_REASSEMBLY_SET | GSM_USER_ID_SET | GSM_USER_PASSWORD_SET |
						GSM_ICMP_SET | GSM_STORED_IN_PROFILE_0 | GSM_FIREWALL_INITIATED |
						GSM_ATCIND_COMMAND_RUNNED ) )
	{
		*gsmStatus |= GSM_INITIATED;
		//writeInt( GSM_INITIATED );
	}	
	else
	{
		*gsmStatus &= ~GSM_INITIATED;
		//writeInt( 0 );
	}

//	DelayMs( 10000 );
	DelayMs( 5000 );

	// Check conection status to mobile operator
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nCheck connection to mobile operator\r\n" );
#endif
	for( i = 0; i < 10; i = i + 1 )
	{
		//TODO Better use gsmConnectedToMobileNetwork( unsigned int* gsmStatus, char* AtCind ).
		if ( gsmGetConnectionToMobileNetworkStatus( AtCind ) )
		{
#ifdef __UART_DEBUG_gsmInit
			int i;
#endif
			*gsmStatus |= GSM_ATCIND_COMMAND_RUNNED;

			/* Print result of 'AT+CIND?' command */
#ifdef __UART_DEBUG_gsmInit
			for( i = 0; i < 18; i++ )
				writeByteToUxRTx( UART_DEBUG, AtCind[i] );
			writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

			//TODO Increment over ',' to reach element 4.
			if( AtCind[4] == '1' )
			{
#ifdef __UART_DEBUG_gsmInit
				writeStringToUxRTx( UART_DEBUG, "\r\nConnected to mobile network\r\n" );
#endif
				*gsmStatus |= GSM_CONNECTED_TO_MOBILE_NETWORK;
				//writeInt( GSM_CONNECTED_TO_MOBILE_NETWORK );
				break;
			}
			else
			{
#ifdef __UART_DEBUG_gsmInit
				writeStringToUxRTx( UART_DEBUG, "Not connected to mobile network\r\n" );
#endif
				*gsmStatus &= ~GSM_CONNECTED_TO_MOBILE_NETWORK;
				//writeInt( 0 );

				// Wait 2s to give the GSM module time to connect to GSM station.
				DelayMs( 2000 );
				continue;
			}
		}
	}
	if( i > 9 )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nERROR: Could not seccessfully run 'AT+CIND?'\r\n" );
#endif
		*gsmStatus &= ~GSM_CONNECTED_TO_MOBILE_NETWORK;
		*gsmStatus &= ~GSM_ATCIND_COMMAND_RUNNED;
#ifdef __UART_DEBUG_gsmInit
		writeInt( 0 );
#endif
//		return 0;
	}

	// Establish GPRS
	if( *gsmStatus & GSM_CONNECTED_TO_MOBILE_NETWORK )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nEstablish GPRS\r\n" );
#endif
		DelayMs( 5000 );

		if( gsmEstablishGPRS())
			*gsmStatus |= GSM_GPRS_ESTABLISHED;
		else
			*gsmStatus &= ~GSM_GPRS_ESTABLISHED;

//		if( *gsmStatus & GSM_GPRS_ESTABLISHED )
//		{
//			// Connect to server
//			writeStringToUxRTx( UART_DEBUG, "\r\nConnect to server\r\n" );
//			if( gsmConnectToServer( 0 ) )
//				*gsmStatus |= GSM_CONNECTED_TO_SERVER;
//			else
//				*gsmStatus &= ~GSM_CONNECTED_TO_SERVER;
//
//			// Get a predetermined file from connected server
//			writeStringToUxRTx( UART_DEBUG, "\r\nGet a predetermined file from connected server\r\n" );
//			if( gsmHttpGet() )
//				*gsmStatus |= GSM_GOT_HTTP;
//			else
//				*gsmStatus &= ~GSM_GOT_HTTP;
//		}
	}

	/*if( !(*gsmStatus & GSM_CONNECTED_TO_MOBILE_NETWORK) )
	{
		writeStringToUxRTx( UART_DEBUG, "\r\nCheck connection to mobile operator\r\n" );
		if( gsmGetConnectionToMobileNetworkStatus( AtCind ) )
		{
			*gsmStatus |= GSM_ATCIND_COMMAND_RUNNED;
			if( AtCind[4] == '1' )
			{
				writeStringToUxRTx( UART_DEBUG, "\r\nConnected to mobile network\r\n" );
				*gsmStatus |= GSM_CONNECTED_TO_MOBILE_NETWORK;
				writeInt( GSM_CONNECTED_TO_MOBILE_NETWORK );
			}	
			else
			{
				writeStringToUxRTx( UART_DEBUG, "Not connected to mobile network\r\n" );
				*gsmStatus &= ~GSM_CONNECTED_TO_MOBILE_NETWORK;
				writeInt( 0 );
			}	
			for( i = 0; i < 18; i++ )
				writeByteToUxRTx( UART_DEBUG, AtCind[i] );
			writeStringToUxRTx( UART_DEBUG, "\r\n" );
		}
		else
		{
			writeStringToUxRTx( UART_DEBUG, "\r\nERROR: Could not seccessfully run 'AT+CIND?'\r\n" );
			*gsmStatus &= ~GSM_CONNECTED_TO_MOBILE_NETWORK;
			*gsmStatus &= ~GSM_ATCIND_COMMAND_RUNNED;
			writeInt( 0 );
		}
	}*/

	return 1;
}

//0 - just the factory profile base section parameters are considered.
//1 - either the factory profile base section and the extended section are considered (full factory profile).
int gsmResetModuleToFactoryDefault( void )
{
	int i;
	int nError = 0;

	for( i = 0; i < 10; i++ )
	{
            if( writeStringToUxRTx( UART_GSM, "AT&F1\r" ) )
                if( waitForString( "OK\r\n", delay3s ) )    // Factory default is result codes in alphanumeric form <CR><LF><text><CR><LF>.
                {
                    nError = 1;
//                    writeStringToUxRTx( UART_DEBUG, "Response format set\r\n" );
                    break;
                }
        }
//        if( i > 3 );
//            error
//		if(gsmSendAtCommand( "&F1\r") )
//#ifdef RESPONSE_FORMAT_RESULT_CODES
//                        if( waitForString( "0\r", delay3s ) )
//#else
//			if( waitForString( "OK\r\n", delay3s ) )
//#endif
//			{
//				nError = 1;
//				break;
//			}

#ifdef __UART_DEBUG_gsmInit
	if( i > 9 )
		writeStringToUxRTx( UART_DEBUG, "Could NOT set module to factory default\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "Could set module to factory default\r\n" );
#endif

	return nError;
}

// 0 - enables result codes (factory default)
// 1 - every result code is replaced with a <CR>
// 2 - disables result codes
int gsmEnableResultCodeLevel( char* level, int maxTries )
{
    int  nError = 0;
    int  i;
    char atCommand[4] __attribute__ ((aligned (8)));
    memcpy( atCommand, "Q", 1 );
    memcpy( atCommand + 1, level, strlen( level ) );
    memcpy( atCommand + 1 + strlen( level ), "\r", 1 );
    memcpy( atCommand + 1 + strlen( level ) + 1, "\0", 1 );

	//     if( gsmSendAtCommand( "Q0\r" ) )
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nCommand to send\r\n\n" );
	writeStringToUxRTx( UART_DEBUG, atCommand );
	writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

    for( i = 0; i < maxTries; i++ )
        if( gsmSendAtCommand( atCommand ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
            if( waitForString( "0\r", delay1s ) )
#else
            if( waitForString( "OK\r\n", delay1s ) )
#endif
            {
                nError = 1;
#ifdef __UART_DEBUG_gsmInit
				writeStringToUxRTx( UART_DEBUG, "Result codes configured\r\n\n" );
#else
                ;
                #endif
                break;
            }

    return nError;
}

// 0 - disables command echo
// 1 - enables command echo (factory default) , hence command sent to the device
// are echoed back to the DTE before the response is given.
int setCommandEchoMode( int echo, int maxTries )
{
    int i = 0;
    int nError = 0;

    if( echo )
    {
        for( i = 0; i < maxTries; i++ )
            if(gsmSendAtCommand( "E1\r\n") )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                if( waitForString( "0\r", delay3s ) )
#else
                if( waitForString( "OK\r\n", delay3s ) )
#endif
                {
					nError = 1;
#ifdef __UART_DEBUG_gsmInit
					writeStringToUxRTx( UART_DEBUG, "Command echo mode set\r\n\n" );
#endif
					break;
                }
    }
    else
    {
        for( i = 0; i < maxTries; i++ )
            if(gsmSendAtCommand( "E0\r\n") )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                if( waitForString( "0\r", delay3s ) )
#else
                if( waitForString( "OK\r\n", delay3s ) )
#endif
                {
                        nError = 1;
                        break;
                }
    }

    return nError;
}

int gsmWaitForOk(BYTE *tmp, int length, unsigned int waitTime)
{
	int nError = 0;
    unsigned int savedCoreTime;
	int i;
//	BYTE tmp[16];
//
//	// Clear string.
//	memset(&tmp,0,sizeof(tmp));

	//DY
	TRACE_CALL_ENTER();

	// Receive answer from GSM module.

	/* Save value of core timer */
	savedCoreTime = ReadCoreTimer();

	WriteCoreTimer( 0 );
//	for(i = 0; (i < length) && (ReadCoreTimer() < waitTime); i++)
	for(i = 0; (i < length) && (ReadCoreTimer() < waitTime);)
	{
//		recUART( UART_GSM, tmp+i );
		if( comm_uart_is_received_data_available( UART_GSM ) )
        {
            *(tmp+i) = comm_uart_get_data_byte( UART_GSM );
			i++;
        }
	}

	/* Restore value in core timer */
	WriteCoreTimer( savedCoreTime );

	// Check received answer.
	if(!memcmp(tmp,"0\r",2)){
		nError = 1;
	}
	if(!memcmp(tmp,"AT\r0\r",4)){
		nError = 1;
	}
	if(!memcmp(tmp,"OK\r\n",4)){
		nError = 1;
	}
	if(!memcmp(tmp,"AT\r\r\nOK\r\n",9)){
		nError = 1;
	}

	TRACE_CALL_EXIT_PARAM_INT(nError);
	return nError;
}

int gsmCheckIfAnsweringOnATCommand(void)
{
//    unsigned int savedCoreTime;
//    int i;
	int length = 16;
	BYTE tmp[length];

	// Clear string.
	memset(&tmp,0,sizeof(tmp));

    //Test sedning 'AT<CR>'.
    if( writeStringToUxRTx( UART_GSM, "AT\r" ) )
    {
        // Receive answer from GSM module.
		if(gsmWaitForOk(tmp, length, delay1s))
			return 1;

//        /* Save value of core timer */
//        savedCoreTime = ReadCoreTimer();
//
//        WriteCoreTimer( 0 );
//        for(i = 0; (i < sizeof(tmp)) && (ReadCoreTimer() < delay1s); i++)
//        {
//            recUART( UART_GSM, tmp+i );
//        }
//
//        /* Restore value in core timer */
//        WriteCoreTimer( savedCoreTime );
//
//        if(!memcmp(tmp,"0\r",2))
//            return 1;
//        if(!memcmp(tmp,"AT\r0\r",4))
//            return 1;
//        if(!memcmp(tmp,"OK\r\n",4))
//            return 1;
//        if(!memcmp(tmp,"AT\r\r\nOK\r\n",9))
//            return 1;

//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//        if( waitForString( "0\r", delay1s ) )
//            return 1;   //Module turned on.
//#else
//        if( waitForString( "OK\r\n", delay1s ) )
//            return 1;   //Module turned on.
//#endif

//                #ifndef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//                    if( waitForString( "0\r", delay1s ) )
//                #else
//                    if( waitForString( "OK\r\n", delay1s ) )
//                #endif
//                        return 1;   // Module turned on and answering.
//                    continue;

//            return 1;   //Module turned on.
    }

    return 0;
}

/* Turn on GSM module */
int gsmTurnOn( int maxTries )
{
	int i;

//	//writeStringToUxRTx( UART_DEBUG, "Check if module is on\r\n" );
//	for( i = 0; i < maxTries; i++ )
//	{
//		if( gsmIsOn() )
//		{
//			writeStringToUxRTx( UART_DEBUG, "Module is on\r\n" );
//			return 1;
//		}
//		else
//		{
//			GSM_RESET = 0;
//			writeStringToUxRTx( UART_DEBUG, "Turning on module\r\n" );
//			nGSM_ON = 1;		// GSM off
//			DelayMs(2000);
//			nGSM_ON = 0;		// GSM On
//		}
//	}
//
//	return 0;
    unsigned int savedCoreTime;
//    int pwrmon_state;

    //Check on/off status pin on module.
//    UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//    pwrmon_state = GSM2CPU_PWRMON_READ;
//    UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//    if( GSM2CPU_PWRMON_READ )

	// Check if PWRMON on GSM module is high, symboling it is on.
    if( gsmReadPwrMon() )
    {
        DelayMs(900);

		// Module is alive if it answers as expected on 'AT\r'.
		if(gsmCheckIfAnsweringOnATCommand())//gsmAnswerOnATCR())
			return 2;
		else
			goto gsmTurnOn_try_again;
//			return 3;   // Module allready on but not answering.
    }
    else
    {
gsmTurnOn_try_again:
        /* Save value of core timer */
        savedCoreTime = ReadCoreTimer();

        // Turn on module.
        for( i = 0; i < maxTries; i = i + 1 )
        {
            WriteCoreTimer( 0 );

            //Reset module.
            //Send turn on signal.
            GMS_ON_SET;

            //Wait for module to turn on, [3;5]s.
//            while( !GSM2CPU_PWRMON_READ && (ReadCoreTimer() < delay5s) );
//            while( ReadCoreTimer() < delay3s );
            DelayMs(2000);

            //Realease turn on signal.
            GMS_ON_CLR;

            //Check on/off status pin on module.
//            UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//            pwrmon_state = GSM2CPU_PWRMON_READ;
//            UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
        //    if( GSM2CPU_PWRMON_READ )
            if( gsmReadPwrMon() )
            {
                int answer = 0;

                DelayMs(900);

                /* Restore value in core timer */
                WriteCoreTimer( savedCoreTime );

                answer = gsmCheckIfAnsweringOnATCommand();
				if(answer)
					return answer;
				else
					gsmHardReset();    //3 tries
			}
            else
            {
//            //Wait for module turning on.
//            WriteCoreTimer( 0 );
//            do
//            {
//                if( ReadCoreTimer() > delay2s )
//                {
                    gsmHardReset();    //3 tries
//                    continue;
//                }
            }
//            while( !gsmIsOn() );


//            while( ReadCoreTimer() < delay1s );

//            /* Restore value in core timer */
//            WriteCoreTimer( savedCoreTime );
        }
    }

    return 0;   //Could not turn module on.
}

int	gsmAnswerOnATCR(void)
{
	int i;
	unsigned int savedCoreTime;
	BYTE tmp[32];

	// Clear string.
	memset(&tmp, 0, sizeof(tmp));

	//Test sedning 'AT<CR>'.
	if( writeStringToUxRTx( UART_GSM, "AT\r" ) )
	{
		/* Save value of core timer */
        savedCoreTime = ReadCoreTimer();

		// Receive answer from GSM module.
		WriteCoreTimer( 0 );
		for(i = 0; (i < sizeof(tmp)) && (ReadCoreTimer() < delay1s); i++)
		{
			recUART( UART_GSM, tmp+i );
		}

		/* Restore value in core timer */
		WriteCoreTimer( savedCoreTime );

		// Check for valis answers.
		if(!memcmp(tmp,"0\r",2))
			return 1;
		if(!memcmp(tmp,"AT\r0\r",4))
			return 1;
		if(!memcmp(tmp,"OK\r\n",4))
			return 1;
		if(!memcmp(tmp,"AT\r\r\nOK\r\n",9))
			return 1;
//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//		if( waitForString( "0\r", delay1s ) )
//			return 1;
//#else
//		if( waitForString( "OK\r\n", delay1s ) )
//			return 1;   //Module turned on.
//#endif
	}

	return 0;	// No answer.
}

/* Switch off GSM module */
int gsmSoftTurnOff( int maxTries )
{
	int i;
//        int pwrmon_state;

//        CPU2GSM_DTR_CLR;

	for( i = 0; i < maxTries; i = i + 1 )
	{
		if( gsmIsOn() )
		{
			int length = 16;
			BYTE tmp[length];

			// Clear string.
			memset(&tmp,0,sizeof(tmp));

			if(!gsmCheckIfAnsweringOnATCommand())
			{
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "Module is not answering\r\n" );
#endif
				return 3;   // The modules PWRMON is high but it does not answer on 'AT\r'.
			}

#ifdef __UART_DEBUG
			writeStringToUxRTx( UART_DEBUG, "Turning off module_1\r\n" );
#endif
			if( gsmSendAtCommand( "#SHDN\r" ) )
			{
				// Receive answer from GSM module.
				if(gsmWaitForOk(tmp, length, delay1s))
//					return 1;
//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
////                                if( waitForString( "0\r", delay2s ) )
//				waitForString( "0\r", delay2s );
//#else
////				if( waitForString( "OK\r\n", delay2s ) )
//			waitForString( "OK\r\n", delay2s );
//#endif
					DelayMs(1200);//1000
			}
		}
		else
		{
                        //Check on/off status of module.
//                        UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//                        pwrmon_state = GSM2CPU_PWRMON_READ;
//                        UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
                    //    if( GSM2CPU_PWRMON_READ )
                        if( gsmReadPwrMon() )
                        {
#ifdef __UART_DEBUG
							writeStringToUxRTx( UART_DEBUG, "\r\nModule is on\r\n" );
#endif
                            return 0;   //Could not turn module off.
                        }
                        else
                        {
#ifdef __UART_DEBUG
                            writeStringToUxRTx( UART_DEBUG, "\r\nModule is off\r\n" );
#endif
                            return 1;   //Module turned off.
                        }
//			return 1;
		}	
	}

	return 0;
}


/* Switch off GSM module */
int gsmHardTurnOff( int maxTries )
{
	int i;
	unsigned int savedCoreTime;
//        int pwrmon_state;

        GMS_ON_CLR;
        GMS_ON_TRIS_CLR;

////	GSM_RESET = 0;
//	gsmHardReset();
//	DelayMs(1000);
//	writeStringToUxRTx( UART_DEBUG, "Turning off module_2\r\n" );
//	nGSM_ON = 1;		// GSM off
//	DelayMs(3000);
//	nGSM_ON = 0;		// GSM On
//	DelayMs(1500);//1000
//
//	for( i = 0; i < maxTries; i = i + 1 )
//	{
//		if( gsmIsOn() )
//		{
//			GSM_RESET = 0;
//			DelayMs(1000);
//			writeStringToUxRTx( UART_DEBUG, "Turning off module_2\r\n" );
//			nGSM_ON = 1;		// GSM off
//			DelayMs(3000);
//			nGSM_ON = 0;		// GSM On
//			DelayMs(1500);//1000
//		}
//		else
//		{
//			writeStringToUxRTx( UART_DEBUG, "Module is off\r\n" );
//			return 1;
//		}
//	}
//
//	return 0;

    /* Save value of core timer */
    savedCoreTime = ReadCoreTimer();

//    if( !GSM2CPU_PWRMON_READ )
    if( !gsmReadPwrMon() )
    {
        DelayMs( 300 );
        return 2;   //Already turned off.
    }

    for( i = 0; i < maxTries; i = i + 1 )
    {
		int answer;

		answer = gsmCheckIfAnsweringOnATCommand();
		if(!answer)	// Modules PWRMON is high but no answer on 'AT\r'.
		{
//			gsmHardReset();		//3 tries
			return 3;			// Modules PWRMON is on but the module is not answering.
		}

#ifdef __UART_DEBUG
        writeStringToUxRTx( UART_DEBUG, "Turning off module_2\r\n" );
#endif

//        GMS_RESET_CLR;
//        GSM2CPU_PWRMON_TRIS_SET;
//        GMS2CPU_ALARM_TRIS_SET;
//        GMS2CPU_RING_TRIS_SET;
//        GMS2CPU_JDR_TRIS_SET;
//        GMS2CPU_DSR_TRIS_SET;
//        CPU2GSM_DTR_TRIS_SET;
//        GMS2CPU_CTS_TRIS_SET;
//        CPU2GMS_RTS_TRIS_SET;
//        GSM_STAT_TRIS_SET;
//        GSM_DISABLET_TRIS_SET;
//        GSM2CPU_RFTXMON_TRIS_SET;
//        GSM2CPU_DCD_TRIS_SET;
        //Send turn off signal.
        GMS_ON_SET;

        //Wait for module to turn off, [3;5]s.
//        WriteCoreTimer( 0 );
//        while( ReadCoreTimer() < delay4s );
//
//        /* Restore value in core timer */
//        WriteCoreTimer( savedCoreTime );
        DelayMs( 4000 );
//writeByteToUxRTx( UART_DEBUG, '1' );DelayMs( 100 );
        //Realease turn off signal.
        GMS_ON_CLR;

//        while( GSM2CPU_PWRMON_READ && (ReadCoreTimer() < delay2s));
//writeByteToUxRTx( UART_DEBUG, '2' );DelayMs( 100 );

//        UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//        pwrmon_state = GSM2CPU_PWRMON_READ;
//        UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    //    if( !GSM2CPU_PWRMON_READ )
        if( !gsmReadPwrMon() )
        {
//writeByteToUxRTx( UART_DEBUG, '3' );DelayMs( 100 );
            return 1;   //Module turned off.
        }
        else
        {
//			int answer;
//
//			answer = gsmCheckIfAnsweringOnATCommand();
//			if(!answer)	// Modules PWRMON is high but no answer on 'AT\r'.
//			{
////				gsmHardReset();		//3 tries
//				return 3;			// Modules PWRMON is on but the module is not answering.
//			}
//writeByteToUxRTx( UART_DEBUG, '4' );//DelayMs( 100 );
            //Wait for module turning off.
//            WriteCoreTimer( 0 );
//            do
//            {
////                if( ReadCoreTimer() > delay15s )
////                {
////                    //Remove power.
////                    WriteCoreTimer( 0 );
////                    //continue;
////                    //gsmSoftReset( 3 );    //3 tries
////                }
//                ;
//writeByteToUxRTx( UART_DEBUG, '5' );DelayMs( 100 );
//                if( ReadCoreTimer() > delay15s )
//                {
//                    writeByteToUxRTx( UART_DEBUG, '6' );DelayMs( 100 );
//                    break;
//                }
//            }
//            while( GSM2CPU_PWRMON_READ && (ReadCoreTimer() < delay15s) );
            for( i = 0; (i < 15) && gsmReadPwrMon(); i++);  // gsmReadPwrMon() internal 1s delay.
//            while( GSM2CPU_PWRMON_READ );
//DelayMs(15000);
//writeByteToUxRTx( UART_DEBUG, '7' );DelayMs( 100 );
        }
//        UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//        pwrmon_state = GSM2CPU_PWRMON_READ;
//        UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    //    if( !GSM2CPU_PWRMON_READ )
        if( !gsmReadPwrMon() )
            break;   //Module turned off.
//writeByteToUxRTx( UART_DEBUG, '8' );DelayMs( 100 );
//        while( ReadCoreTimer() < delay1s );
//        while( ReadCoreTimer() < delay500ms );


    }

    /* Restore value in core timer */
    WriteCoreTimer( savedCoreTime );
//writeByteToUxRTx( UART_DEBUG, '9' );DelayMs( 100 );

    //Check on/off status of module.

//    if( GSM2CPU_PWRMON_READ )
    if( gsmReadPwrMon() )
    {
#ifdef __UART_DEBUG
        writeStringToUxRTx( UART_DEBUG, "\r\nModule is on\r\n" );
#endif
        return 0;   //Could not turn module off.
    }
    else
    {
#ifdef __UART_DEBUG
        writeStringToUxRTx( UART_DEBUG, "\r\nModule is off\r\n" );
#endif
        return 1;   //Module turned off.
    }

    return 0;   //Could not turn module off.
}

int gsmReadPwrMon( void )
{
    int pwrmon_state;

    UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    DelayMs( 1000 );
    pwrmon_state = GSM2CPU_PWRMON_READ;
    DelayMs( 10 );
    UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

    return pwrmon_state;
}
/* Reset GSM module */	
int gsmSoftReset( int maxTries )
{
	int i;

#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "Module soft reset\r\n" );
#endif

	for( i = 0; i < maxTries; i = i + 1 )
		if( gsmSendAtCommand( "Z" ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        if( waitForString( "0\r", delay100ms ) )
#else
			if( waitForString( "OK\r\n", delay100ms ) )
#endif
				return 1;

	return 0;
}

/* Reset GSM module */	
int gsmHardReset( void )
{
//	int pwrmon_state;

	GMS_RESET_TRIS_CLR;

#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "Module hard reset\r\n" );
#endif

	//Turn on Reset signal.
	UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//	GSM_RESET = 1;
	GMS_RESET_SET;

	DelayMs(250);

	//Turn off Reset signal.
//	GSM_RESET = 0;
	GMS_RESET_CLR;

	DelayMs(2000);
	UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

        //Check on/off status of module.
//        UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//        pwrmon_state = GSM2CPU_PWRMON_READ;
//        UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    //    if( GSM2CPU_PWRMON_READ )
	if( gsmReadPwrMon() )
	{
		while( ReadCoreTimer() < delay1s );

		//Test sedning  AT<CR>
//		if( writeStringToUxRTx( UART_GSM, "AT\r" ) )
//		{
//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//			if( waitForString( "0\r", delay1s ) )
//
//#else
//			if( waitForString( "OK\r\n", delay1s ) )
//#endif
//		}
//		else
//			return 0;
		if(gsmCheckIfAnsweringOnATCommand())
			return 1;   //Module turned on.
		else
			return 0;
	}
	else
	{
		while( ReadCoreTimer() < delay1s );

		if( gsmTurnOn( 1 ) ) //1 try.
			return 1;
		else
			return 0;
	}
}

int gsmShowInitStatus( unsigned int* gsmStatus )
{
#ifdef __UART_DEBUG_gsmInit
	int i;
#endif

	// GSM test summary
#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, " - GSM test summary -\r\n" );
	for( i = 1; i < 0x3000000000000; i = 2 * i )	// Max value of unsigned int is 0x4000000000000
	{
		if( i > GSM_ATCIND_COMMAND_RUNNED )
			break;

		switch( i )
		{
			case GSM_INITIATED:
							writeStringToUxRTx( UART_DEBUG, "GSM_INITIATED                   " ); break;
			case GSM_ON:
							writeStringToUxRTx( UART_DEBUG, "GSM_ON                          " ); break;
			case GSM_IPR_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_IPR_SET                     " ); break;
			case GSM_FLOW_CONTROL_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_FLOW_CONTROL_SET            " ); break;
			case GSM_COMMAND_LEVEL_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_COMMAND_LEVEL_SET           " ); break;
			case GSM_ERROR_LEVEL_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_ERROR_LEVEL_SET             " ); break;
			/*case GSM_SIM_CARD_INSERTED:
							writeStringToUxRTx( UART_DEBUG, "GSM_SIM_CARD_INSERTED           " ); break;*/
			case GSM_TE_CHARACTER_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_TE_CHARACTER_SET            " ); break;
			case GSM_TCP_REASSEMBLY_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_TCP_REASSEMBLY_SET          " ); break;
			case GSM_USER_ID_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_USER_ID_SET                 " ); break;
			case GSM_USER_PASSWORD_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_USER_PASSWORD_SET           " ); break;
			case GSM_ICMP_SET:
							writeStringToUxRTx( UART_DEBUG, "GSM_ICMP_SET                    " ); break;
			case GSM_STORED_IN_PROFILE_0:
							writeStringToUxRTx( UART_DEBUG, "GSM_STORED_IN_PROFILE_0         " ); break;
			case GSM_STORED_IN_PROFILE_1:
							writeStringToUxRTx( UART_DEBUG, "GSM_STORED_IN_PROFILE_1         " ); break;
			case GSM_FIREWALL_INITIATED:
							writeStringToUxRTx( UART_DEBUG, "GSM_FIREWALL_INITIATED          " ); break;
			case GSM_CONNECTED_TO_MOBILE_NETWORK:
							writeStringToUxRTx( UART_DEBUG, "GSM_CONNECTED_TO_MOBILE_NETWORK " ); break;
			case GSM_GPRS_INITIATED:
							writeStringToUxRTx( UART_DEBUG, "GSM_GPRS_INITIATED              " ); break;
			case GSM_GPRS_ESTABLISHED:
							writeStringToUxRTx( UART_DEBUG, "GSM_GPRS_ESTABLISHED            " ); break;
			case GSM_CONNECTED_TO_SERVER:
							writeStringToUxRTx( UART_DEBUG, "GSM_CONNECTED_TO_SERVER         " ); break;
			case GSM_IMEI:
							writeStringToUxRTx( UART_DEBUG, "GSM_IMEI                        " ); break;
			case GSM_IMSI:
							writeStringToUxRTx( UART_DEBUG, "GSM_IMSI                        " ); break;
			case GSM_SOCKET_CLOSED:
							writeStringToUxRTx( UART_DEBUG, "GSM_SOCKET_CLOSED               " ); break;
			case GSM_GOT_HTTP:
							writeStringToUxRTx( UART_DEBUG, "GSM_GOT_HTTP                    " ); break;
			case GSM_ATCIND_COMMAND_RUNNED:
							writeStringToUxRTx( UART_DEBUG, "GSM_ATCIND_COMMAND_RUNNED       " ); break;
			default:
							i *= 2;	continue;													  break;
		}
		//gsmStatus >> (31 - i)) & 0x1
		//if( *gsmStatus & ( 1 << i ) )
		/*if( (*gsmStatus >> i) & 0x1 )
			writeByteToUxRTx( UART_DEBUG, '1' );
		else
			writeByteToUxRTx( UART_DEBUG, '0' );*/
		( (*gsmStatus & i) ? writeByteToUxRTx( UART_DEBUG, '1' ) : writeByteToUxRTx( UART_DEBUG, '0' ));
		//( (gsmStatus >> (31 - i)) & 0x1 ? writeByteToUxRTx( UART_DEBUG, '1' ) : writeByteToUxRTx( UART_DEBUG, '0' ));

		writeStringToUxRTx( UART_DEBUG, "\r\n" );
	}
#endif


#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nGSM status string:\r\n" );
	for( i = 0; i < 32; i++ )
		( (*gsmStatus >> (31 - i)) & 0x1 ? writeByteToUxRTx( UART_DEBUG, '1' ) : writeByteToUxRTx( UART_DEBUG, '0' ));
	writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

#ifdef __UART_DEBUG_gsmInit
	// Check if GSM module was proparly  configured
	if( (*gsmStatus & (	GSM_ON | GSM_IPR_SET | GSM_FLOW_CONTROL_SET | GSM_COMMAND_LEVEL_SET |
						GSM_ERROR_LEVEL_SET | GSM_SIM_CARD_INSERTED | GSM_TE_CHARACTER_SET |
						GSM_TCP_REASSEMBLY_SET | GSM_USER_ID_SET | GSM_USER_PASSWORD_SET |
						GSM_ICMP_SET | GSM_STORED_IN_PROFILE_0 |
						GSM_FIREWALL_INITIATED |
						GSM_GPRS_INITIATED | GSM_GPRS_ESTABLISHED | GSM_CONNECTED_TO_SERVER |
						GSM_IMEI | GSM_IMSI | GSM_GOT_HTTP |
						GSM_ATCIND_COMMAND_RUNNED ) )
			==		(	GSM_ON | GSM_IPR_SET | GSM_FLOW_CONTROL_SET | GSM_COMMAND_LEVEL_SET |
						GSM_ERROR_LEVEL_SET | GSM_SIM_CARD_INSERTED | GSM_TE_CHARACTER_SET |
						GSM_TCP_REASSEMBLY_SET | GSM_USER_ID_SET | GSM_USER_PASSWORD_SET |
						GSM_ICMP_SET | GSM_STORED_IN_PROFILE_0 |
						GSM_FIREWALL_INITIATED |
						GSM_GPRS_INITIATED | GSM_GPRS_ESTABLISHED | GSM_CONNECTED_TO_SERVER |
						GSM_IMEI | GSM_IMSI | GSM_GOT_HTTP |
						GSM_ATCIND_COMMAND_RUNNED ) )

		writeStringToUxRTx( UART_DEBUG, "\r\nGSM module was proparly configured\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "\r\nGSM module was NOT properly  configured\r\n" );
#endif

#ifdef __UART_DEBUG_gsmInit
	if( *gsmStatus & GSM_CONNECTED_TO_MOBILE_NETWORK )
		writeStringToUxRTx( UART_DEBUG, "GSM module was connected to mobile network\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "GSM module was NOT connected to mobile network\r\n" );
#endif

	return 1;
}

int gsmIsOn( void )
{
//    int pwrmon_state;
//	int nError = 0;

//	clrUARTRxBuffer( UART_GSM );

//	if( writeStringToUxRTx( UART_GSM, "AT\r" ) )
//#ifdef RESPONSE_FORMAT_RESULT_CODES
//                if( waitForString( "0\r", delay3s ) )
//#else
//		if( waitForString( "OK\r\n", delay3s ) )
//#endif
    //Check on/off status pin on module.
//    UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//    pwrmon_state = GSM2CPU_PWRMON_READ;
//    UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//    if( GSM2CPU_PWRMON_READ )
    if( gsmReadPwrMon() )
        return 1;
    else
        return 0;
//			nError = 1;
//	if( writeStringToUxRTx( UART_GSM, "AT\r" ) )
//	{
//		if( waitForString( "AT", delay3s ) )
//		{
//			nError = 1;
//			waitForString( "OK\r\n", delay100ms );
//		}
//	}

//	return nError;
}

int gsmGetListeningStatus( char* socket )
{
    int  nError = 0;
//    int  j,l;
    char command[64]    __attribute__ ((aligned (4)));
    char string[16]     __attribute__ ((aligned (4)));
//    BYTE c              __attribute__ ((aligned (4)));

    // Clear string.
    memset( command, 0, strlen( command ));
    memset( string , 0, strlen( string  ));

    // Build up command string.
    memcpy( command, "#SL?\r", 5 );
//    memcpy( command + 4, socket, 1 );
//    memcpy( command + 5, "\r", 1 );
#ifdef __UART_DEBUG
    writeStringToUxRTx( UART_DEBUG, "\r\nCommand:  AT" );
    writeStringToUxRTx( UART_DEBUG, command );
    writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

    if( gsmSendAtCommand( command ) )
    {
        if( waitForString( "#SL: 1", delay250ms ) )
        {
            nError = 1;

            // Wait for command to finnish.
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
            waitForString( "0\r", delay3s );
#else
            waitForString( "OK\r\n", delay3s );
#endif
        }
        else
            nError = 0;

#ifdef __UART_DEBUG
		if( nError )
			writeStringToUxRTx( UART_DEBUG, "\r\nWe are listening for incoming call\r\n" );
		else
			writeStringToUxRTx( UART_DEBUG, "\r\nWe are NOT listening for incoming call\r\n" );
#endif
    }
    else
        // Could not send command "AT#SL?".
        nError = -1;

    return nError;
}

int gsmPendingIncomingConnection( char* socket )
{
#define l_command_length 64
#define l_string_length 32
    int  nError = 0;
    int  j,l;
    char command[l_command_length]    __attribute__ ((aligned (4)));
    char string[l_string_length]     __attribute__ ((aligned (4)));
    BYTE c              __attribute__ ((aligned (4)));

    // Clear string.
    memset( command, 0, l_command_length);
    memset( string , 0, l_string_length);

    // Build up command string.
    memcpy( command, "#SS=", 4 );
    memcpy( command + 4, socket, 1 );
    memcpy( command + 5, "\r", 1 );
#ifdef __UART_DEBUG
    writeStringToUxRTx( UART_DEBUG, "\r\nCommand:  AT" );
    writeStringToUxRTx( UART_DEBUG, command );
    writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

    if( gsmSendAtCommand( command ) )
    {
        if( waitForString( "#SS: ", delay250ms ) )
        {
            // Save status in variable
            for( j = 0; j < 32; j++ )
                if( recUART( UART_GSM, &c ) )	/* recUART() has internally delay1s */
                {
                        string[j] = c;
                        //writeByteToUxRTx( UART_DEBUG, c ); //HB
                }
                else
                    break;

            // Wait for command to finnish.
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//            if( !waitForString( "0\r", delay3s ) )
            waitForString( "0\r", delay250ms );
#else
//            if( !waitForString( "OK\r\n", delay3s ) )
            waitForString( "OK\r\n", delay3s );
#endif

            /* Print result of 'AT#SS=<socket>' command */
#ifdef __UART_DEBUG
			for( j = 0; j < 32; j++ )
			{
				writeByteToUxRTx( UART_DEBUG, string[j] );
				if( (string[j] == '\r') && (string[j] == '\n') )
					break;
			}
			writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

            /* Step over two ',' in AtCind                  */
            /* With this we will reach element 3 in AtCind */
            /* At first check variable l will be zero       */
            l = -1;
            for( j = 0; (j < 1) && (l < (16-1)); )	/* Parse by two ',' */
            {
                    do
                    {
                            l = l + 1;
                    }while( (string[l] != ',') && (l < 16 - 1) );
                    j = j + 1;
            }

            // Check if socket state is:
            // '5 - Socket with an incoming connection.
            //  Waiting for the user accept or shutdown command.'

//                if( string[l + 1] == '5' )
            if( string[2] == '5' )
            {
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nPending incoming call\r\n" );
#endif
				nError = 5;
            }
            if( string[2] == '1' )
            {
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nPending incoming call\r\n" );
#endif
				nError = 1;
            }
            if( !nError )
            {
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nNo pending incoming call\r\n" );
#endif
				nError = 0;
            }
        }
        else
            // We did not resieve "#SS: ".
//            nError = -1;
            nError = 0;
    }
    else
        // Could not send command "AT#SS=<socket>".
//        nError = -1;
        nError = 0;

    return nError;
#undef l_command_length
#undef l_string_length
}

// Use with causion!
int gsmSetIPR( char* baudRate )
{
    UARTSetDataRate(UART_GSM, GetPeripheralClock(), 9600);	//TODO Why here?!?
	int i, j;
	int nError = 0;
//        int ok = 0;
        char atCommand[14] __attribute__ ((aligned (8)));

        memset(atCommand,0,sizeof(atCommand));
        if(!memcmp(baudRate,"9600",4)) {
            memcpy( atCommand, "+IPR=9600\r", 10 );
        } else if(!memcmp(baudRate,"115200",4)) {
            memcpy( atCommand, "+IPR=115200\r", 10 );
        }
//        memcpy( atCommand, "+IPR=", 5 );
//        memcpy( atCommand + 5, baudRate, strlen( baudRate ) );
//        memcpy( atCommand + 5 + strlen( baudRate ), "\r", 1 );
//        memcpy( atCommand + 5 + strlen( baudRate ) + 1, "\0", 1 );

#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, atCommand );
#endif

        clrUARTRxBuffer( UART_GSM );

	for( i = 0; i < 4; i++ )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+IPR?\r\n" );
#endif
		/*writeByteToUxRTx( UART_GSM, 'A' );
		writeByteToUxRTx( UART_GSM, 'T' );
		if( waitForString( "AT", delay1s ) )
		{
			writeStringToGSM( "+IPR?\r" );*/
			if( gsmSendAtCommand( "+IPR?\r" ) )
			if( waitForString( "+IPR: ", delay1s ) )
			{
				//recUART( UART_GSM, &c );
				break;
			}
		//}
	}
//        if(!memcmp(baudRate,"115200",4))
        if( !waitForString( baudRate, delay1s ) )
//            if( !waitForString( "115200", delay1s ) )
//                ok = 1;
//	if( !waitForString( "57600", delay1s ) )
//	if( !waitForString( "19200", delay1s ) )
//        if(!memcmp(baudRate,"9600",4))
//            if( !waitForString( "9600", delay1s ) )
//                ok = 1;
//        if(!ok)
	{
		for( i = 0; i < 4; i++ )
		{
			for( j = 0; j < 4; j++ )
                            if( gsmSendAtCommand( atCommand ) )
//                          if( gsmSendAtCommand( "+IPR=115200\r" ) )
//                        if( gsmSendAtCommand( "+IPR=57600\r" ) )
//			if( gsmSendAtCommand( "+IPR=19200\r" ) )
//                                if( waitForString( "9600", delay1s ) )
//                                    if( waitForString( "OK\r\n", delay1s) )
//                        if( gsmSendAtCommand( "+IPR=9600\r" ) )
                        //if( gsmSendAtCommand( "+IPR=9600\r" ) )
                        {
//                            while( ((uartReg[UART_GSM]->sta.reg)&0) || !((uartReg[UART_GSM]->sta.reg)&(1<<8)) );	// Defined in <peripheral/uart.h>
//                            UARTEnable(UART_GSM, UART_DISABLE_FLAGS(UART_ENABLE|UART_PERIPHERAL | UART_RX | UART_TX));
//        /* OK???? */                   UARTSetDataRate(UART_GSM, GetPeripheralClock(), *baudRate);
//                            UARTEnable(UART_GSM, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
//                            UARTConfigure(UART_GSM, UART_ENABLE_PINS_CTS_RTS);
//                            UARTSetDataRate(UART_GSM, GetPeripheralClock(), 115200);
//                                UARTSetDataRate(UART_GSM, GetPeripheralClock(), 57600);
//				UARTSetDataRate(UART_GSM, GetPeripheralClock(), 19200);
//                                UARTSetDataRate(UART_GSM, GetPeripheralClock(), 9600);
//				if( writeStringToUxRTx( UART_GSM, "AT" ) )
//				if( waitForString( "AT", delay1s ) )
//					if( writeStringToGSM( "+IPR=19200\r" ) )
//					//if( writeStringToGSM( "+IPR=0\r" ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                                if( waitForString( "0\r", delay1s ) )
#else
                                                if( waitForString( "OK\r\n", delay1s) )
#endif
                                                        goto gsmSetIprOk;
                        }
                        return nError;
			gsmSetIprOk:

				// Check Baudrate
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+IPR?\r\n" );
#endif
			if( gsmSendAtCommand( "+IPR?\r" ) )
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
			{
//				writeStringToGSM( "+IPR?\r" );
                                if( waitForString( baudRate, delay1s ) )
//				if( waitForString( "+IPR: 115200", delay3s ) )
//				if( waitForString( "+IPR: 57600", delay3s ) )
//				if( waitForString( "+IPR: 19200", delay3s ) )
//				if( waitForString( "+IPR: 9600", delay3s ) )
//				//if( waitForString( "+IPR: 0", delay1s ) )
				{
                                    nError = 1;
                                    break;
				}
			}	
		}
	}
	else
		nError = 1;	

	// Wait for command respons to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        waitForString( "0\r", delay1s );
#else
	waitForString( "OK\r\n", delay1s );
#endif

	return nError;
}

//// HB changed 20111223
//int gsmSetFlowControl( void )
//{
//	int nError = 0;
//	
//	// Set flowcontol
//	if( gsmSendAtCommand( "\\Q0\r" ) )
//			if( waitForString( "OK\r\n", delay1s) )
//				nError = 1;
////	if( writeStringToUxRTx( UART_GSM, "AT" ) )
////	{
////		if( waitForString( "AT", delay1s ) )
////		{
////			writeStringToGSM( "\\Q0\r" );
////			if( waitForString( "OK\r\n", delay1s) );
////				nError = 1;
////		}
////	}
//	
//	return nError;		
//}
// HB changed 20120104
int gsmSetFlowControl( void )
{
	int i;
	int nError = 1;

	// Set flowcontol
	for( i = 0; i < 4; i++ )
	{
//		if( gsmSendAtCommand( "\\Q0\r" ) )  //no flow control.
//		if( gsmSendAtCommand( "\\Q1\r" ) )  //software bi-directional with filtering (XON/XOFF).
//		if( gsmSendAtCommand( "\\Q2\r" ) )  //hardware mono-directional flow control (only CTS active).
                if( gsmSendAtCommand( "\\Q3\r" ) )  //hardware bi-directional flow control (both RTS/CTS active) (factory default).

#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                    if( waitForString( "0\r", delay1s ) )
#else
                    if( waitForString( "OK\r\n", delay1s) )
#endif
                        break;
	}
	if( i > 3 )
		nError = 0;

	return nError;		
}

//// HB changed 20111223
//int gsmSetCommandLevel( void )
//{
//	char c;
//	int i;
//	int nError = 0;
//	
//	for( i = 0; i < 4; i++ )
//	{
//		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SELINT?\r\n" );
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		if( waitForString( "AT", delay1s ) )
//		{
//			writeStringToGSM( "#SELINT?\r" );
//			if( waitForString( "#SELINT: ", delay1s ) )
//			{
//				recUART( UART_GSM, &c );
//				writeByteToUxRTx(UART_DEBUG, c );
//				break;
//			}
//		}		
//	}
//	// Wait for command response to finish
//	waitForString( "OK\r\n", delay1s );
//	
//	if( c != '2' )
//	{
//		for( i = 0; i < 4 ; i++ )
//		{
//			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SELINT=2\r\n" );
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "#SELINT=2\r" );
//				// Wait for "OK"
//				if( !waitForString( "OK\r\n", delay1s ) )
//				{
//					i++;
//					continue;
//				}
//			}	
//			    
//		    // Check GPRS if connection is opened
//			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SELINT?\r\n" );
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "#SELINT?\r" );
//				if( waitForString( "#SELINT: ", delay1s ) )
//				{
//				    recUART( UART_GSM, &c );
//					writeByteToUxRTx(UART_DEBUG, c );
//					
//					// Wait for command response to finish
//					waitForString( "OK\r\n", delay1s );
//					if( c == '2' )
//					{
//						nError = 1;
//						break;
//					}	
//				}
//			}	
//		}
//		if( i > 3 )
//			nError = 0;
//		else
//			nError = 1;
//	}
//	else
//		nError = 1;
//	
//	if( nError )
//		writeStringToUxRTx( UART_DEBUG, "Command level set to 2\r\n" );
//	else
//		writeStringToUxRTx( UART_DEBUG, "ERROR: Could not set command level 2\r\n" );
//	
//	return nError;
//}
// HB changed 20111223
int gsmSetCommandLevel( void )
{
//	char	c	__attribute__ ((aligned (8))); //REMOVED
	BYTE	c	__attribute__ ((aligned (8))); //ADDED
	int		i,j;
	int		nError = 0;

#ifdef __UART_DEBUG_gsmInit
	writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SELINT?\r\n" );
#endif
	for( i = 0; i < 4; i++ )
	{
		if( gsmSendAtCommand( "#SELINT?\r" ) )
			if( waitForString( "#SELINT: ", delay1s ) )
				break;
	}
	recUART( UART_GSM, &c );

	// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        waitForString( "0\r", delay1s );
#else
	waitForString( "OK\r\n", delay1s );
#endif

        // Check command level in use and possibly correct it.
	if( c != '2' )
	{
		for( i = 0; i < 4; i++ )
		{
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SELINT=2\r\n" );
#endif
			for( j = 0; j < 4; j++ )
				if( gsmSendAtCommand( "#SELINT=2\r" ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                                        if( waitForString( "0\r", delay1s ) )
#else
							if( waitForString( "OK\r\n", delay1s) )
#endif
								goto gsmSetCommandLevelOk;
		    return nError;
			gsmSetCommandLevelOk:

		    // Check the new command-level
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SELINT?\r\n" );
#endif
			if( gsmSendAtCommand( "#SELINT?\r" ) )
			{
				if( waitForString( "#SELINT: 2", delay3s ) )
				{
					nError = 1;
					break;
				}
			}	
		}

		if( i > 3 )
			nError = 0;
		else
			nError = 1;
	}
	else
		nError = 1;

#ifdef __UART_DEBUG_gsmInit
	if( nError )
		writeStringToUxRTx( UART_DEBUG, "Command level set to 2\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "ERROR: Could not set command level 2\r\n" );
#endif

	return nError;
}

// HB changed 20111223
int gsmSetErrorLevel( void )
{
	int		i;
//	char	c	__attribute__ ((aligned (8))) = 0; //REMOVED
	BYTE	c	__attribute__ ((aligned (8))) = 0; //ADDED
	int		nError = 0;

	for( i = 0; i < 4; i++ )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+CMEE?\r\n" );
#endif
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		if( waitForString( "AT", delay1s ) )
//		{
//			writeStringToUxRTx( UART_GSM, "+CMEE?\r" );
//			if( waitForString( "+CMEE: ", delay1s ) )
//			{
//				// Get the command level character/byte
//			    recUART( UART_GSM, &c );
//				writeByteToUxRTx(UART_DEBUG, c );
//				break;
//			}
//		}
		if( gsmSendAtCommand( "+CMEE?\r" ) )
			if( waitForString( "+CMEE: ", delay1s ) )
			{
				recUART( UART_GSM, &c );
				break;
			}	
	}
	// Wait for command response to finish
	//waitForString( "OK\r\n" );

        // Check the Report Mobile Equipment Error in use.
	if( c != '0' )
	{
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "0\r", delay1s );
#else
		waitForString( "OK\r\n", delay1s );
#endif
		for( i = 0; i < 4 ; i++ )
		{
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+CMEE=0\r\n" );
#endif
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			waitForString( "AT", delay1s );
//			writeStringToGSM( "+CMEE=0\r" );
			if( gsmSendAtCommand( "+CMEE=0\r" ) )
				// Wait for "OK"
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                if( waitForString( "0\r", delay1s ) )
#else
				if( waitForString( "OK\r\n", delay1s ) )
#endif
					goto gsmSetErrorLevel_OK;
			i++;
			continue;

gsmSetErrorLevel_OK:
		    // Check the new Report Mobile Equipment Error.
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+CMEE?\r\n" );
#endif
			if( gsmSendAtCommand( "+CMEE?\r" ) )
				if( waitForString( "+CMEE: ", delay1s ) )
				{
					recUART( UART_GSM, &c );
					break;
				}
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			waitForString( "AT", delay1s );
//			writeStringToGSM( "+CMEE?\r" );
//			if( waitForString( "+CMEE: ", delay1s ) )
//			{
//			    recUART( UART_GSM, &c );
//				writeByteToUxRTx(UART_DEBUG, c );
//
//				// Wait for command response to finish
//				waitForString( "OK\r\n", delay1s );
//				if( c == '0' )
//				{
//					nError = 1;
//					break;
//				}
//			}
                    if( c == '0' )
                    {
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        if( waitForString( "0\r", delay1s ) )
#else
                        if( waitForString( "OK\r\n", delay1s ) )
#endif
                            nError = 1;
                        break;
                    }
		}
		if( i > 3 )
			nError = 0;
	}
	else
	{
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
            if( waitForString( "0\r", delay1s ) )
#else
            if( waitForString( "OK\r\n", delay1s ) )
#endif
                nError = 1;
	}	

#ifdef __UART_DEBUG_gsmInit
	if( nError )
		writeStringToUxRTx( UART_DEBUG, "\r\nDevice error level set\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "ERROR: Could not set devices error level\r\n" );
#endif


	return nError;
}

// Automatic Time Zone update - +CTZU.
// Network Timezone - #NITZ.
int gsmUpdateTimeFromGsmNetwork(char updateTimeForNewtwork)
{
	int status = 0;
	int length = 16;
	BYTE tmp[length];

	////	Page 155-156
	////	AT+CTZU=<onoff>
	////	0 Disable automatic time zone update via NITZ (default)
	////	1 Enable automatic time zone update via NITZ
	if(gsmSendAtCommand("+CTZU="))	// Disabled.
	{
		writeByteToUxRTx( UART_GSM, updateTimeForNewtwork );
		writeByteToUxRTx( UART_GSM, '\r' );

		if(gsmWaitForOk(tmp, length, delay1s))
			status |= 1;
	}

	////	Page 317-320
	////	Network Timezone - #NITZ
	////	AT#NITZ=[<val>[,<mode>]]
	if(gsmSendAtCommand("#NITZ="))	// Disabled.
	{
		writeByteToUxRTx( UART_GSM, updateTimeForNewtwork );
		writeByteToUxRTx( UART_GSM, '\r' );

		if(gsmWaitForOk(tmp, length, delay1s))
			status |= 2;
	}

	return status;
}

int gsmCheckSimCardInserted( char* IMSI )
{
	#ifdef __UART_DEBUG_gsmInit
	int i;
	#endif
	int nError = 0;

	gsm_get_imsi( IMSI );
	if( IMSI[0] )
	{
		nError = 1;
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSIM card inserted\r\n" );
		writeStringToUxRTx( UART_DEBUG, "IMSI = " );
		for( i = 0; i < 15; i++)
			writeByteToUxRTx( UART_DEBUG, IMSI[i] );
		writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif
	}	
	else
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nInsert SIM card\r\n" );
#else
		;
#endif

	return nError;
}

// HB changed 20111223
int gsmSetTeCharacterSet( void )
{
	int i,j;
	int nError = 0;

	for( i = 0; i < 4; i++ )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+CSCS?\r\n" );
#endif
		if( gsmSendAtCommand( "+CSCS?\r" ) )
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		if( waitForString( "AT", delay1s ) )
		{
//			writeStringToGSM( "+CSCS?\r" );
			if( waitForString( "+CSCS: ", delay1s ) )
			{
				//recUART( UART_GSM, &c );
				break;
			}
			DelayMs( 150 ); // If no success,wait a little longer before next try. Is this waiting needed?
		}	
	}
	if( !waitForString( "IRA", delay1s ) )
	{
		for( i = 0; i < 4; i++ )
		{
			for( j = 0; j < 4; j++ )
                        {
				if( gsmSendAtCommand( "+CSCS=\"IRA\"\r" ) )
//				if( writeStringToUxRTx( UART_GSM, "AT" ) )
//					if( waitForString( "AT", delay1s ) )
//						if( writeStringToGSM( "+CSCS=\"IRA\"\r" ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                                        if( waitForString( "0\r", delay1s ) )
#else
							if( waitForString( "OK\r", delay1s) )
#endif
								goto gsmSetIraOk;
                        }
                        return nError;
                            gsmSetIraOk:

#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+CSCS?\r\n" );
#endif
			if( gsmSendAtCommand( "+CSCS?\r" ) )
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
			{
//				writeStringToGSM( "+CSCS?\r" );
				if( waitForString( "+CSCS: \"IRA\"", delay1s ) )
				{
					nError = 1;
					break;
				}
			}	
		}
	}
	else
		nError = 1;	

	// Wait for command respons to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        waitForString( "0\r", delay1s );
#else
	waitForString( "OK\r\n", delay1s );
#endif

	return nError;
}

// HB changed 20111223
int gsmSetTcpReassembly( void )
{
	int		i;
//	char	c	__attribute__ ((aligned (8))); ///REMOVED
	BYTE	c	__attribute__ ((aligned (8))); //ADDEA
	int		nError = 0;

	for( i = 0; i < 4; i++ )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nCheck for TCP reassembly?\r\n" );
#endif
		if( gsmSendAtCommand( "#TCPREASS?\r" ) )
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		if( waitForString( "AT", delay1s ) )
//		{
//			writeStringToUxRTx( UART_GSM, "#TCPREASS?\r" );
			if( waitForString( "#TCPREASS: ", delay1s ) )
			{
				// Get the TCPREASS character/byte
			    recUART( UART_GSM, &c );
#ifdef __UART_DEBUG_gsmInit
				writeByteToUxRTx(UART_DEBUG, c );
#endif
				break;
			}
//		}
	}
	// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        waitForString( "0\r", delay1s );
#else
	waitForString( "OK\r\n", delay1s );
#endif

	if( c != '1' )
	{
		for( i = 0; i < 4 ; i++ )
		{
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#TCPREASS=1\r\n" );
#endif
			if( gsmSendAtCommand( "#TCPREASS=1\r" ) )
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "#TCPREASS=1\r" );
				// Wait for "OK"
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                if( !waitForString( "0\r", delay1s ) )
#else
				if( !waitForString( "OK\r\n", delay1s ) )
#endif
				{
					i++;
					continue;
				}
//			}

		    // Check GPRS if connection is opened
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#TCPREASS?\r\n" );
#endif
			if( gsmSendAtCommand( "#TCPREASS?\r" ) )
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "#TCPREASS?\r" );
				if( waitForString( "#TCPREASS: ", delay1s ) )
				{
				    recUART( UART_GSM, &c );
#ifdef __UART_DEBUG_gsmInit
					writeByteToUxRTx(UART_DEBUG, c );
#endif

					// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                        waitForString( "0\r", delay1s );
#else
					waitForString( "OK\r\n", delay1s );
#endif
					if( c == '1' )
					{
						nError = 1;
						break;
					}
//				}
			}
		}
		if( i > 3 )
			nError = 0;
	}
	else
		nError = 1;

#ifdef __UART_DEBUG_gsmInit
	if( nError )
		writeStringToUxRTx( UART_DEBUG, "TCP reassembly set\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "ERROR: Could not set TCP reassembly\r\n" );
#endif

	return nError;
}

// HB changed 20111223
int gsmSetUserId( void )
{
	//int nError = 0;
	int i,j;
//	char c;
	int nError = 0;

	for( i = 0; i < 4; i++ )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#USERID=\"\"\r\n" );
#endif
		if( gsmSendAtCommand( "#USERID?\r" ) )
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		if( waitForString( "AT", delay1s ) )
//		{
//			writeStringToGSM( "#USERID?\r" );
			if( waitForString( "#USERID: ", delay1s ) )
			{
				//recUART( UART_GSM, &c );
				break;
			}
//		}
	}
	if( !waitForString( "\"\"", delay1s ) )
	{
		for( i = 0; i < 4; i++ )
		{
			for( j = 0; j < 4; j++ )
                        {
				if( gsmSendAtCommand( "#USERID=\"\"\r" ) )
//				if( writeStringToUxRTx( UART_GSM, "AT" ) )
//					if( waitForString( "AT", delay1s ) )
//						if( writeStringToGSM( "#USERID=\"\"\r" ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                                        if( waitForString( "0\r", delay1s ) )
#else
							if( waitForString( "OK\r\n", delay1s ) )
#endif
								goto gsmSetUserIdOk;
                        }
                        return nError;
                            gsmSetUserIdOk:

		    // Check Baudrate
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#USERID?\r\n" );
#endif
			if( gsmSendAtCommand( "#USERID?\r" ) )
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "#USERID?\r" );
				if( waitForString( "USERID: \"\"", delay1s ) )
				{
					nError = 1;
					// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                        waitForString( "0\r", delay1s );
#else
					waitForString( "OK\r\n", delay1s );
#endif
					break;
				}
//			}
		}
	}
	else
	{
		nError = 1;
		// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "0\r", delay1s );
#else
		waitForString( "OK\r\n", delay1s );
#endif
	}	

#ifdef __UART_DEBUG_gsmInit
	if( nError )
		writeStringToUxRTx( UART_DEBUG, "\r\nCleared user id\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "ERROR: Could not clear user id\r\n" );
#endif

	return nError;
}

// HB changed 20111223
// Can not check user password, just set it (or clear it).
int gsmSetUserPassword( void )
{
	int i;
	int nError = 0;

	for( i = 0; i < 4; i++ )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#PASSW=\"\"\r\n" );
#endif
		if( gsmSendAtCommand( "#PASSW=\"\"\r" ) )
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		if( waitForString( "AT", delay1s ) )
//		{
//			writeStringToGSM( "#PASSW=\"\"\r" );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        if( waitForString( "0\r", delay1s ) )
#else
			if( waitForString( "OK\r\n", delay1s ) )
#endif
			{
				//recUART( UART_GSM, &c );
				nError = 1;
				break;
			}
//		}
	}
#ifdef __UART_DEBUG_gsmInit
	if( !nError )
		writeStringToUxRTx( UART_DEBUG, "WARNING: Password might not have been cleared\r\n" );
#endif

	return nError;
}

// HB changed 20111223
//int gsmSetIcmp( void )
int gsmSetIcmp( char icmpAcceptance )
{
	//int nError = 0;

	int		i;
//	char	c	__attribute__ ((aligned (8))); //REMOVED
	BYTE	c	__attribute__ ((aligned (8))); //ADDED
	int		nError = 0;

	for( i = 0; i < 4; i++ )
	{
#ifdef __UART_DEBUG_gsmInit
		writeStringToUxRTx( UART_DEBUG, "\r\nChecking ICMP rule?\r\n" );
#endif
		if( gsmSendAtCommand( "#ICMP?\r" ) )
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		if( waitForString( "AT", delay1s ) )
//		{
//			writeStringToUxRTx( UART_GSM, "#ICMP?\r" );
			if( waitForString( "#ICMP: ", delay1s ) )
			{
			    recUART( UART_GSM, &c );
#ifdef __UART_DEBUG_gsmInit
				writeByteToUxRTx(UART_DEBUG, c );
#endif
				break;
			}
//		}
	}
	// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        waitForString( "0\r", delay1s );
#else
	waitForString( "OK\r\n", delay1s );
#endif

//	if( c != '1' )
	if( c != icmpAcceptance )
	{
		for( i = 0; i < 4 ; i++ )
		{
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#ICMP=1\r\n" );
#endif
//			if( gsmSendAtCommand( "#ICMP=1\r" ) )
			if( gsmSendAtCommand( "#ICMP=" ) )
                        {
                            writeByteToUxRTx( UART_GSM, icmpAcceptance );
                            writeByteToUxRTx( UART_GSM, '\r' );
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "#ICMP=1\r" );
				// Wait for "OK"
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                if( !waitForString( "0\r", delay1s ) )
#else
				if( !waitForString( "OK\r\n", delay1s ) )
#endif
				{
					i++;
					continue;
				}
//			}
                        }

		    // Check GPRS if connection is opened
#ifdef __UART_DEBUG_gsmInit
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#ICMP?\r\n" );
#endif
			if( gsmSendAtCommand( "#ICMP?\r" ) )
//			writeByteToUxRTx( UART_GSM, 'A' );
//			writeByteToUxRTx( UART_GSM, 'T' );
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "#ICMP?\r" );
				if( waitForString( "#ICMP: ", delay1s ) )
				{
					recUART( UART_GSM, &c );
#ifdef __UART_DEBUG_gsmInit
					writeByteToUxRTx(UART_DEBUG, c );
#endif

					// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                        waitForString( "0\r", delay1s );
#else
					waitForString( "OK\r\n", delay1s );
#endif
//					if( c == '1' )
					if( c == icmpAcceptance )
					{
						nError = 1;
						break;
					}
				}
//			}
		}
		if( i > 3 )
			nError = 0;
	}
	else
		nError = 1;

#ifdef __UART_DEBUG_gsmInit
	if( nError )
		writeStringToUxRTx( UART_DEBUG, "ICMP rule set\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "ERROR: Could not set ICMP rule\r\n" );
#endif

	return nError;
}

// HB changed 20111223
int gsmStoreInProfile0( void )
{
	int i;
	int nError = 0;

	for( i = 0; i < 4; i++ )
	{
		if( gsmSendAtCommand( "&W0\r" ) )
//		if( writeStringToUxRTx( UART_GSM, "AT" ) )
//		{
//			if( waitForString( "AT", delay1s ) )
//			{
//				writeStringToGSM( "&W0\r" );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                if( waitForString( "0\r", delay1s ) )
#else
				if( waitForString( "OK\r\n", delay1s) )
#endif
				{
					nError = 1;
					break;
				}	
//			}
//		}
	}

	return nError;
}


int gsmSetClock( int posixTime, int maxTries )
{
	int i;
	int nError = 0;
	char clockString[25]	__attribute__ ((aligned (32)));

	//for( i = 0; i < 8; i++ )
	//posixTime = 1046844428;

	/* Convert to the GSM module's time format  */
	posix2telit_time( clockString, (time_t)posixTime);

	//writeStringToUxRTx( UART_DEBUG, (char*)posixString );
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, clockString );
#endif
//	writeStringToUxRTx( UART_DEBUG, "+CCLK=\"" );
//gsmSendAtCommand(               "+CCLK=\"" );
//writeStringToUxRTx( UART_DEBUG, "03/04/05,06:07:08+00" );
//writeStringToUxRTx( UART_GSM,   "03/04/05,06:07:08+00" );
//writeStringToUxRTx( UART_DEBUG, "\"\r\n" );
//writeStringToUxRTx( UART_GSM,   "\"\r" );
//waitForString( "OK\r\n", delay3s );

	/* Update the GSM modules clock */
	for( i = 0; i < maxTries; i++ )
	{
		if( gsmSendAtCommand( "+CCLK=\"" ) )
			if( writeStringToUxRTx( UART_GSM, clockString ) )
				if( writeStringToUxRTx( UART_GSM, "\"\r" ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                        if( waitForString( "0\r", delay1s ) )
#else
					if( waitForString( "OK\r\n", delay1s) )
#endif
					{
						nError = 1;
						break;
					}
	}

	return nError;
}

int gsmGetClock( int* posixTime, int maxTries )
{
	int i,j;
	int nError = 1;

//	char sc					__attribute__ ((aligned (8))); //REMOVED
	BYTE uc					__attribute__ ((aligned (8))); //ADDED
	char clockString[26]	__attribute__ ((aligned (32)));
	char* pClockString		__attribute__ ((aligned (8)))	= clockString;

	/* Make sure string ends with NULL */
	memset( clockString, 0, 26 );

	for( i = 0; i < maxTries; i++ )
		if( gsmSendAtCommand( "+CCLK?\r" ) )
			break;

	//if( i < 4 )
	if( i < maxTries )
	{
		if( waitForString( "+CCLK: \"", delay100ms ) )
		{
			//while(1)
			for( j = 0; j < 25; j++ )
			{
				if( recUART( UART_GSM, &uc ) )	// 1 second internal time out in function.
#ifdef __UART_DEBUG
					writeByteToUxRTx( UART_DEBUG, uc );
#else
					;
#endif
				else
					nError = 0;
				if( uc == '"' )
					break;

				/* Add next character to clock-string */
				*pClockString++ = uc;
			}
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
			waitForString( "0\r", delay100ms );
#else
			waitForString( "OK\r\n", delay100ms );
#endif
		}		
	}

	/* If we could retrieve the clock value from the GSM module we update our time variable */
	if( nError )
		*posixTime = telit_time2posix( clockString );

	return nError;
}

int gsmSetFirewallRule( char* ip_address, char* net_mask, int max_tryouts )
{
    int  nError = 0;
    int  i;
    char command[64] __attribute__ ((aligned (4)));

    // Clear string.
    memset( command, 0, strlen( command ));

    // Build up command string.
    memcpy( command, "#FRWL=1,\"", 9 );
    memcpy( command + 9, ip_address, 15 );
    memcpy( command + 24, "\",\"", 3 );
    memcpy( command + 27, net_mask, 15 );
    memcpy( command + 42, "\"\r", 2 );
#ifdef __UART_DEBUG
    writeStringToUxRTx( UART_DEBUG, "\r\nCommand:  AT" );
    writeStringToUxRTx( UART_DEBUG, command );
    writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

    for( i = 0; i < max_tryouts; i++ )
    {
        if( gsmSendAtCommand( command ) )
        {
    //                if( gsmSendAtCommand( "#FRWL=1,\"83.251.136.93\",\"255.255.255.000\"\r" ) ) // Erik home.
            // Wait for command to finnish.
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
            if( !waitForString( "0\r", delay3s ) )
#else
            if( !waitForString( "OK\r\n", delay3s ) )
#endif
            {
                // Check firewall for the newly inserted rule.
                if( gsmSendAtCommand( "#FRWL?\r" ) )
                {
                    if( waitForString( ip_address, delay1s ) )
                    {
                        // Wait for command to finnish.
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        waitForString( "0\r", delay1s );
#else
                        waitForString( "OK\r\n", delay1s );
#endif
                        nError = 1;
                        break;
                    }
                }
            }
            else
                nError = 0;
        }
    }

    return nError;
}

int gsmSetFirewallInit( void )
{
	int i;//,j;
	int nError = 0;

	// Clear firewall
	for( i = 0; i < 4; i++ )
	{
		/*if( writeStringToUxRTx( UART_GSM, "AT" ) )
			if( waitForString( "AT", delay3s ) )
				if( writeStringToGSM( "#FRWL=2\r" ) )
					if( waitForString( "OK\r\n", delay3s ) )
					{
						nError = 1;
						break;
					}
		*/
        // Remove all chaines.
		if( gsmSendAtCommand( "#FRWL=2\r" ) )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        if( waitForString( "0\r", delay3s ) )
#else
			if( waitForString( "OK\r\n", delay3s ) )
#endif
			{
				nError = 1;
				break;
			}

	}
	if( !nError )
		return nError;

	// Reset nError
	nError = 0;

	// Add firewall rules.
	for( i = 0; i < 4; i++ )
	{
		// HB
		/* Changed to new function below */
		/*if( writeStringToUxRTx( UART_GSM, "AT" ) )
			if( waitForString( "AT", delay3s ) )
				if( writeStringToGSM( "#FRWL=1,\"213.115.113.054\",\"255.255.255.000\"\r" ) )
					if( !waitForString( "OK\r\n", delay3s) )
					{
						i++;
						continue;
					}
		*/
            // String sizes must be 15 chars.
            // Strings must be of format [xxx.xxx.xxx.xxx].
            DelayMs(1000);
            gsmSetFirewallRule( "192.168.001.087", "255.255.255.000", 4 );  // AW home.
            gsmSetFirewallRule( "213.115.113.054", "255.255.255.000", 4 );  // Office.
            gsmSetFirewallRule( "083.251.136.093", "255.255.255.000", 4 );  // Erik home.
            gsmSetFirewallRule( "213.112.177.074", "255.255.248.000", 4 );  // Henrik home.
            gsmSetFirewallRule( "094.234.178.001", "255.255.255.000", 4 );  // Mobile phone.
//		if( gsmSendAtCommand( "#FRWL=1,\"213.115.113.054\",\"255.255.255.000\"\r" ) )
////                if( gsmSendAtCommand( "#FRWL=1,\"83.251.136.93\",\"255.255.255.000\"\r" ) ) // Erik home.
//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//                        if( !waitForString( "0\r", delay3s ) )
//#else
//			if( !waitForString( "OK\r\n", delay3s ) )
//#endif
//			{
//				i++;
//				continue;
//			}

		// HB
		/* Changed to new function below */
		/*if( writeStringToUxRTx( UART_GSM, "AT" ) )
			if( waitForString( "AT", delay3s ) )
				if( writeStringToGSM( "#FRWL?\r" ) )
					if( waitForString( "#FRWL: ", delay3s ) )
						break;*/
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nCheck Firewall rules:\r\n" );
#endif
		if( gsmSendAtCommand( "#FRWL?\r" ) )
			break;
	}
        waitForString( "???", delay1s );
//	if( !waitForString( "\"213.115.113.054\",\"255.255.255.000\"", delay3s ) )
//	{
//		for( i = 0; i < 4; i++ )
//		{
//			for( j = 0; j < 4; j++ )
//                        {
//				// HB
//				/* Changed to new function below */
//				/*if( writeStringToUxRTx( UART_GSM, "AT" ) )
//					if( waitForString( "AT", delay3s ) )
//						if( writeStringToGSM( "#FRWL=1,\"213.115.113.054\",\"255.255.255.000\"\r" ) )
//							if( waitForString( "OK\r\n", delay3s ) )
//								goto gsmSetFirewallInitOk;*/
//				if( gsmSendAtCommand( "#FRWL=1,\"213.115.113.054\",\"255.255.255.000\"\r" ) )
//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//                                        if( waitForString( "0\r", delay3s ) )
//#else
//					if( waitForString( "OK\r\n", delay3s ) )
//#endif
//						goto gsmSetFirewallInitOk;
//                        }
//
//                        return nError;
//                                gsmSetFirewallInitOk:
//
//		    // Check rules
//		    #ifdef __UART_DEBUG
//			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#FRWL?\r\n" );
//			#endif
//			// HB
////			//TODO Change to new function below, gsmSendAtCommand().
////			if( writeStringToUxRTx( UART_GSM, "AT" ) )
////				if( waitForString( "AT", delay3s ) )
////					if( writeStringToGSM( "#FRWL?\r" ) )
////						if( waitForString( "#FRWL: \"213.115.113.054\",\"255.255.255.000\"", delay3s ) )
////						{
////							// Wait for command response to finish
////							nError = 1;
////#ifdef RESPONSE_FORMAT_RESULT_CODES
////                                                        waitForString( "0\r", delay3s );
////#else
////							waitForString( "OK\r\n", delay3s );
////#endif
////							break;
////						}
//		if( gsmSendAtCommand( "#FRWL?\r" ) )
//			if( waitForString( "#FRWL: \"213.115.113.054\",\"255.255.255.000\"", delay3s ) )
//			{
//				// Wait for command response to finish
//				nError = 1;
//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//                                waitForString( "0\r", delay3s );
//#else
//				waitForString( "OK\r\n", delay3s );
//#endif
//				break;
//			}
//		}
//	}
//	else
//	{
//		nError = 1;
//		// Wait for command response to finish
//#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//                waitForString( "0\r", delay3s );
//#else
//		waitForString( "OK\r\n", delay3s );
//#endif
//	}

	return nError;
}

int gsmGetConnectionToMobileNetworkStatus( char* AtCind )
{
	int		i,j;
//	int i;
//	char	c	__attribute__ ((aligned (8))); //REMOVED
	BYTE	c	__attribute__ ((aligned (8))); //ADDED
	int		nError = 0;

	// Clear statusvariable
	memset( AtCind, 0, 22 );

	for( i = 0; i < 4; i++ )
	{
            if( gsmSendAtCommand( "+CIND?\r" ) )
            {
                if( waitForString( "+CIND: ", delay100ms ) )
                {
                    // Save status in variable
                    for( j = 0; j < 22; j++ )
                        if( recUART( UART_GSM, &c ) )	/* recUART() has internally delay1s */
                        {
                                AtCind[j] = c;
                                //writeByteToUxRTx( UART_DEBUG, c ); //HB
                        }
    //                            else
    //                                break;
                    break;
                }
            }
            //else
            DelayMs( 10 );
        }

        // Did we successfully executed the test command
        if( AtCind[17] != 0 )	/* Number of carachters if successfully executed is [18;22] */
                nError = 1;

        if( nError )
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "0\r", delay1s );
#else
                waitForString( "OK\r\n", delay1s );
#endif

        return nError;
}

int gsmConnectedToMobileNetwork( unsigned int* gsmStatus, char* AtCind )
{
	int i,j,l;

	for( i = 0; i < 1; i++ )
	{
		if( gsmGetConnectionToMobileNetworkStatus( AtCind ) ) //Fills the AtCind status variable
		{
			/* Print result of 'AT+CIND?' command */
#ifdef __UART_DEBUG
			for( j = 0; j < 18; j++ )
				writeByteToUxRTx( UART_DEBUG, AtCind[j] );
			writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

			/* Step over two ',' in AtCind                  */
			/* With this we will reach element 3 in AtCind */
			/* At first check variable l will be zero       */
			l = -1;
			for( j = 0; (j < 2) && (l < (22-1)); )	/* Parse by two ',' */
			{
				do
				{
					l = l + 1;
				}while( (AtCind[l] != ',') && (l < 22 - 1) );
				j = j + 1;
			}

			if( AtCind[l + 1] == '1' )
			{
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nConnected to mobile network\r\n" );
#endif
				*gsmStatus |= GSM_CONNECTED_TO_MOBILE_NETWORK;
				//writeInt( GSM_CONNECTED_TO_MOBILE_NETWORK );
				break;
			}	
			else
			{
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "Not connected to mobile network\r\n" );
#endif
				*gsmStatus &= ~GSM_CONNECTED_TO_MOBILE_NETWORK;
				//writeInt( 0 );
//				DelayMs( 2000 );
				continue;
			}	
		}
		else
			*gsmStatus &= ~GSM_CONNECTED_TO_MOBILE_NETWORK;
	}

	return (*gsmStatus & GSM_CONNECTED_TO_MOBILE_NETWORK);
}

int gsmInitGprs(char* CGDCONT) {
  int i;
  int nError = 0;

  // AT#SCFG=<connId/socketId>,<cid/PDP>,<pktSz>,<maxTo>,<connTo>,<txTo>
  //         socket id 1-6, {context 0, GSM 1-5 GPRS}, default packet size, exchange info timeout in seconds, connection timeout in 100ms, time out till send without full packet in 100ms.
  // //AT#PKTSZ=<NrOfBytes> -> Set the default packet size to be used by the TCP/UDP/IP stack.

  for (i = 0; i < 4; i++) {
    //if( gsmSendAtCommand( "#SCFG=1,1,300,300,600,50\r" ) )	/* 5 seconds Tx time-out */
    //if( gsmSendAtCommand( "#SCFG=1,1,300,300,600,10\r" ) )		/* 1 second  Tx time-out */
    if (gsmSendAtCommand("#SCFG=1,1,300,90,600,10\r")) { /* 300B package max size, 90s exchange timeout, 60s connection timeout, 1 second  Tx time-out */
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
      if (waitForString("0\r", delay3s))
#else
      if (waitForString("OK\r\n", delay3s))
#endif
        break;
    }
  }
  //TODO Include check for SCFG.
  //if( i > 3 )
  //	nError = 0;

  clrUARTRxBuffer( UART_GSM);
  // AT+CGDCONT=1,�IP�,�APN�,�10.10.10.10�,0,0
  // AT+CGDCONT: 1,"IP","public.telenor.se","0.0.0.0",0,0
#ifdef __UART_DEBUG
  writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+CGDCONT=");	//1,\"IP\",\"public.telenor.se\",\"0.0.0.0\",0,0" );
  writeStringToUxRTx( UART_DEBUG, CGDCONT);
  writeStringToUxRTx( UART_DEBUG, "\r\n");
#endif
  /*for( i = 0; i < 4; i++ )
   if( writeStringToUxRTx( UART_GSM, "AT" ) )
   if( waitForString( "AT", delay1s ) )
   if( writeStringToGSM( "+CGDCONT=" ) )
   if( writeStringToGSM( CGDCONT ) )
   if( writeStringToGSM( "\r" ) )
   if( waitForString( "OK\r\n", delay1s) )
   break;*/
  for (i = 0; i < 4; i++) {
    if (gsmSendAtCommand("+CGDCONT=")) {
      if (writeStringToGSM(CGDCONT)) {
        if (writeStringToGSM("\r")) {
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
          if (waitForString("0\r", delay5s))
#else
            if( waitForString( "OK\r\n", delay5s ) )
#endif
            break;
        }
      }
    }
  }
  //if( i > 3 )
  //	nError = 0;

  /*for( i = 0; i < 4; i++ )
   if( writeStringToUxRTx( UART_GSM, "AT" ) )
   if( waitForString( "AT", delay1s ) )
   if( writeStringToGSM( "+CGDCONT?\r" ) )
   if( waitForString( CGDCONT, delay1s ) )
   if( waitForString( "OK\r\n", delay1s) )
   break;*/
  //TODO Include below in above for().
  for (i = 0; i < 4; i++) {
    if (gsmSendAtCommand("+CGDCONT?\r"))
      if (waitForString(CGDCONT, delay3s))
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        if (waitForString("0\r", delay5s))
#else
          if( waitForString( "OK\r\n", delay5s ) )
#endif
          break;
  }

  if (i < 3) {
    nError = 1;
  }

  return nError;
}

int gsmEstablishGPRS( void )
{
//	char			c	__attribute__ ((aligned (8))); //REMOVED
	BYTE			c	__attribute__ ((aligned (8))); //ADDED
	int				i;
	int				nError = 1;
	unsigned int	savedCoreTime;

	// Check GPRS connection is opened
	// Clear GSM modules input buffer
	/*writeByteToUxRTx( UART_GSM, '\r' );
	DelayMs( 10 );
	clrUARTRxBuffer( UART_GSM );*/
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SGACT?\r\n" );
#endif
	/*writeByteToUxRTx( UART_GSM, 'A' );
	writeByteToUxRTx( UART_GSM, 'T' );
	waitForString( "AT", delay1s );
	writeStringToUxRTx( UART_GSM, "#SGACT?\r" );*/
	gsmSendAtCommand( "#SGACT?\r" );
	// Wait for "#SGACT: 1,"
	if( !waitForString( "#SGACT: 1,", delay5s ) )
	{
		return 0;	// Did not receive expected string in time.
	}

	// Get the connection on/off character/byte
	recUART( UART_GSM, &c );
#ifdef __UART_DEBUG
	writeByteToUxRTx(UART_DEBUG, c );
#endif

	// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        waitForString( "0\r", delay1s );
#else
	waitForString( "OK\r\n", delay3s );
#endif

	if( c == '0' )
	{
		for( i = 0; i < 8 ; i++ )	// Not properly/fully opened. AT#SGACT? returns #SGACT: 1,0. Must return #SGACT: 1,1 to be opened
		{
			if( i == 0 )
				gsmInitGprs( "1,\"IP\",\"online.telia.se\",\"0.0.0.0\",0,0" );
			if( i == 1 )
				gsmInitGprs( "1,\"IP\",\"static.telenor.se\",\"0.0.0.0\",0,0" );
//			if( i == 2 )
//				gsmInitGprs( "1,\"IP\",\"services.telenor.se\",\"0.0.0.0\",0,0" );
			if( i == 2 )
				gsmInitGprs( "1,\"IP\",\"internet.telenor.se\",\"0.0.0.0\",0,0" );
			if( i == 3 )
				gsmInitGprs( "1,\"IP\",\"public.telenor.se\",\"0.0.0.0\",0,0" );
			if( i == 4 )
				gsmInitGprs( "1,\"IP\",\"online.telia.se\",\"0.0.0.0\",0,0" );
			if( i == 5 )
				gsmInitGprs( "1,\"IP\",\"static.telenor.se\",\"0.0.0.0\",0,0" );
//			if( i == 3 )
//				gsmInitGprs( "1,\"IP\",\"services.telenor.se\",\"0.0.0.0\",0,0" );
			if( i == 6 )
				gsmInitGprs( "1,\"IP\",\"internet.telenor.se\",\"0.0.0.0\",0,0" );
			if( i == 7 )
				gsmInitGprs( "1,\"IP\",\"public.telenor.se\",\"0.0.0.0\",0,0" );

			DelayMs( 10 );

#ifdef __UART_DEBUG
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SGACT=1,1\r\n" );
#endif
			/*writeByteToUxRTx( UART_GSM, 'A' );
			writeByteToUxRTx( UART_GSM, 'T' );
			writeStringToGSM( "#SGACT=1,1\r" );*/
			gsmSendAtCommand( "#SGACT=1,1\r" );

			// Wait for "OK"
			/*if( !waitForString( "OK\r\n", delay30s ) )
				continue;*/

#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
			waitForString( "0\r", delay30s );
#else
			waitForString( "OK\r\n", delay30s );
#endif

			// Check GPRS if connection is opened
#ifdef __UART_DEBUG
			writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SGACT?\r\n" );
#endif
			//writeByteToUxRTx( UART_GSM, 'A' );
			//writeByteToUxRTx( UART_GSM, 'T' );
			//writeStringToGSM( "#SGACT?\r" );
			gsmSendAtCommand( "#SGACT?\r" );
			// Wait for "#SGACT: 1,"
			waitForString( "#SGACT: 1,", delay3s );


			/* Save value of core timer */
			savedCoreTime = ReadCoreTimer();
			/* Set timer value to 0 */
			WriteCoreTimer( 0 );

			// Get the connection on/off character/byte
			recUART( UART_GSM, &c );
			//do
			//{
			//	if( UARTReceivedDataIsAvailable( UART_DEBUG ) )
			//	{
			//		c = UARTGetDataByte( UART_DEBUG );
			//		break;
			//	}	
			//}while( ReadCoreTimer() < delay1s );

			/* Restore value in core timer */
			WriteCoreTimer( savedCoreTime );

#ifdef __UART_DEBUG
			writeByteToUxRTx(UART_DEBUG, c );
#endif

			// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        waitForString( "0\r", delay1s );
#else
			waitForString( "OK\r\n", delay3s );
#endif

			if( c == '1' )
				break;
		}
		if( i >= 8 )
			nError = 0;
	}

#ifdef __UART_DEBUG
	if( nError )
		writeStringToUxRTx( UART_DEBUG, "\r\nConnected through GPRS\r\n" );
	else
		writeStringToUxRTx( UART_DEBUG, "\r\nERROR: setting up GPRS connection\r\n" );
#endif

	return nError;
}

int gsmCheckGprsEstablished( unsigned int* gsmStatus )
{
	int		i;
//	char	c	__attribute__ ((aligned (8))); //REMOVED
	BYTE	c	__attribute__ ((aligned (8))); //ADDED
	int		nError = 0;

	for( i = 0; i < 10; i = i + 1 )
	{
		if( gsmSendAtCommand( "#SGACT?\r" ) )
		{
			if( waitForString( "#SGACT: 1,", delay5s ) )
			{
				/* Get the connection on/off character/byte */
			    recUART( UART_GSM, &c );
#ifdef __UART_DEBUG
				writeByteToUxRTx(UART_DEBUG, c );
#endif

				// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                waitForString( "0\r", delay3s );
#else
				waitForString( "OK\r\n", delay3s );
#endif
				if( c == '1' )
					nError = 1;
			}
			break;
		}
	}

	if( nError )
		*gsmStatus |= GSM_GPRS_INITIATED;
	else
		*gsmStatus &= ~GSM_GPRS_INITIATED;

	return nError;
}

// HB changed 20111223
// UNTESTED
// TODO - Test that it actually works
int gsmCloseGPRS( void )
{
//	char	c __attribute__ ((aligned (8))) = 1; //REMOVED
	BYTE	c __attribute__ ((aligned (8))) = 1; //ADDED
	int		i;

	// Close GPRS connection
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nClosing GPRS connection\r\n" );
	//	c = 1;
	writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SGACT?\r\n" );
#endif
	for( i = 0; i < 10; i++ )
	{
		if( gsmSendAtCommand( "#SGACT?\r" ) )
		{
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		writeStringToGSM( "#SGACT?\r" );
//		// Wait for "#SGACT: 1,"
			waitForString( "#SGACT: 1,", delay1s );

		// Get the connection on/off character/byte
		    recUART( UART_GSM, &c );
#ifdef __UART_DEBUG
			writeByteToUxRTx(UART_DEBUG, c );
#endif
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        waitForString( "0\r", delay1s );
#else
			waitForString( "OK\r\n", delay1s );
#endif
			if( c == '0' )
				return 1;		/* GPRS connection already closed */
		}
	}

	for( i = 0; i < 10; i++ )
//	do	// Not properly/fully closed. AT#SGACT? returns #SGACT: 1,1. Must return #SGACT: 1,0 to be closed
	{
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SGACT=1,0\r\n" );
#endif
		if( gsmSendAtCommand( "#SGACT=1,0\r" ) )
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		writeStringToGSM( "#SGACT=1,0\r" );
		// Wait for "OK"
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                    waitForString( "0\r", delay1s );
#else
                    waitForString( "OK\r\n", delay1s );
#endif
		else
		{
			i++;
			continue;
		}	

	    // Check GPRS connection is closed
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nSending AT#SGACT?\r\n" );
#endif
		if( gsmSendAtCommand( "#SGACT?\r" ) )
		{
//		writeByteToUxRTx( UART_GSM, 'A' );
//		writeByteToUxRTx( UART_GSM, 'T' );
//		writeStringToGSM( "#SGACT?\r" );
//		// Wait for "#SGACT: 1,"
			waitForString( "#SGACT: 1,", delay1s );

		// Get the connection on/off character/byte
		    recUART( UART_GSM, &c );
#ifdef __UART_DEBUG
			writeByteToUxRTx(UART_DEBUG, c );
#endif
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                        waitForString( "0\r", delay1s );
#else
			waitForString( "OK\r\n", delay1s );
#endif
			if( c == '0' )
				break;
		}
		// Wait for command response to finish
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "0\r", delay1s );
#else
		waitForString( "OK\r\n", delay1s );
#endif
	}
//	while( c != '0' );
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "GPRS connection closed\r\n" );
#endif

	if( i < 10 )
		return 1;
	else
		return 0;
}

int gsmConnectToServer( char* server, char* port, int maxTries, int commandMode )
{
	int i;
	int nError = 0;

	// Connect to remote server
	for( i = 0; i < maxTries; i++ )
		if( gsmSendAtCommand( "#SD=1,0," ) )
			if( writeStringToGSM( port ) )
				if( writeStringToGSM( ",\"" ) )
					if( writeStringToGSM( server ) )
						if( writeStringToGSM( "\",0,0,") )
						{
							if( commandMode )
							{
								writeStringToGSM( "1\r" );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
								if( waitForString( "0\r", delay50s ) )   // TODO Is this enough?
#else
								if( waitForString( "OK", delay50s ) )
#endif
								{
									nError = 1;
									break;
								}
							}
							else
							{
								writeStringToGSM( "0\r" );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
								if( waitForString( "1\r", delay50s ) )   // TODO Is this enough?
#else
								if( waitForString( "CONNECT", delay50s ) )
#endif
								{
									nError = 1;
									break;
								}
							}
						}

	return nError;
}

int gsmConnectToServer_Multi( int val )
{
//	int i;
	int nError = 0;

	// Connect to remote server
	switch( val )
	{
		case 0:
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nConnecting to atcindu.carlssonia.org:81\r\n" );
#endif
				if( gsmConnectToServer( "atcindu.carlssonia.org", "81", 4, 0 ) )
//				for( i = 0; i < 4; i++ )
//					if(gsmSendAtCommand( "#SD=1,0,81,atcindu.carlssonia.org,0,0\r" ) )
//						if( waitForString( "CONNECT", delay50s ) )
				{
					nError = 1;
					break;
				}
				break;
		case 1:
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nConnecting to www.isk.kth.se:80\r\n" );
#endif
				if( gsmConnectToServer( "www.isk.kth.se", "80", 4, 0 ) )
//				for( i = 0; i < 4; i++ )
//					if(gsmSendAtCommand( "#SD=1,0,80,www.isk.kth.se,0,0\r") )
//						if( waitForString( "CONNECT", delay50s ) )
				{
					nError = 1;
					break;
				}
				break;
		case 2:
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nConnecting to hem.bredband.net:80\r\n" );
#endif
				if( gsmConnectToServer( "hem.bredband.net", "80", 4, 0 ) )
//				for( i = 0; i < 4; i++ )
//					if(gsmSendAtCommand( "#SD=1,0,80,hem.bredband.net,0,0\r" ) )
//						if( waitForString( "CONNECT", delay50s ) )
				{
					nError = 1;
					break;
				}
				break;
	};

	return nError;
}

/* The IMEI buffer provided needs to be 16 bytes long (including NULL at the end) */
void gsm_get_imei(char* IMEI) {
  int i = 0;
  BYTE c __attribute__ ((aligned (8)));

  LOGD("Sending AT+CGSN - Requesting the IMEI");

  //Request Product Serial Number Identification
  for (i = 0; i < 4; i++) {
    if (gsmSendAtCommand("+CGSN\r")) {
      // We are in sync with the module, let us continue with waiting for the command response.
      break;
    }
  }

  /* Wait for IMEI string */
  do {
    recUART( UART_GSM, &c);
  } while (c == '\r' || c == '\n');

  /* Catch the IMEI string */
  for (i = 0; i < 15; i++) {
    /* Check for error message - for instance SIM card not inserted */
    if (c == 'E' || c == 'e') {
      memset(IMEI, 0, 16);
      waitForString("RROR\r", delay1s);
      return;
    }

    /* Build up the IMEI number */
    IMEI[i] = (char) c;

    /* Receive next digit */
    recUART( UART_GSM, &c);
  }

  gsm_wait_for_response(GSM_RESPONSE_CODE_OK, delay1s);

  /* Do something with this - set a flag */
}

void gsm_get_imsi( char* IMSI )
{
	int				i,j;
	unsigned int	savedCoreTime;
//	char			c	__attribute__ ((aligned (8))); //REMOVED
	BYTE			c	__attribute__ ((aligned (8))); //ADDED

	/* The string IMSI is pointing to needs to be 15 digits long */

        // Clear string.
        memset( IMSI, 0, 16);
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nSending AT+CIMI - Requesting the IMSI\r\n" );
#endif
	for( j = 0; j < 4; j++ )
	{
		DelayMs( 1000 );

		for( i = 0; i < 4; i++ )
			if( gsmSendAtCommand( "+CIMI\r" ) )
						break;

		/* Wait for IMSI string */
		savedCoreTime = ReadCoreTimer();
		do
		{
			recUART( UART_GSM, &c );
		}while( (c == '\r' || c ==	'\n') && ReadCoreTimer() < delay250ms );
		WriteCoreTimer( savedCoreTime );

		/* Get IMSI string */
		for( i = 0; i < 15; i++ )
		{
#ifdef __UART_DEBUG
			writeByteToUxRTx( UART_DEBUG, c );
#endif
			/* Check for error message - for instance SIM card not inserted */
			if( c == 'E' || c == 'e' )
			{
				IMSI[0] = 0;
				waitForString( "RROR", delay1s );
				break;//return;
			}

			/* Build up the IMSI number */
			IMSI[i] = (char) c;

			/* Receive next digit */
			recUART( UART_GSM, &c );
		}
		/* If we did not receive 15 digits try again */
		if( IMSI[0] )
			break;
	}	

	gsm_wait_for_response(GSM_RESPONSE_CODE_OK, delay1s);
	/* Do something with this - set a flag */
}


int gsmHttpGet( void )
{
	int nError;

	// Send HTTP/GET to request file
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nSending HTTP/GET\r\n" );
#endif
	writeStringToUxRTx( UART_GSM, "GET /~hborg/test http/1.1\r\nHost: www.isk.kth.se\r\n\r\n" );
	//writeStringToUxRTx( UART_GSM, "GET /henrikborg/HenrikBorg.asc http/1.1\r\nHost: home.bredband.net\r\n\r\n" );
	// Wait for </html>
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nRetreiving file and Wating for </html>\r\n" );
#endif
	if( waitForString( "</html>", delay50s ) )
	{
		nError = 1;
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "Got </html>\r\n" );
#endif
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "3\r", delay10s );
#else
		waitForString( "NO CARRIER\r", delay10s );
#endif
	}	
	else
	{
		nError = 0;
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nDit not retreive HTML-file\r\n" );
#endif
	}

	return nError;
}

// HB changed 20111223
int gsmAtcindCommand( char* AtCind )
{
	int		i, j;
//	char	c       __attribute__ ((aligned (8))); //REMOVED
	BYTE	c       __attribute__ ((aligned (8))); //ADDED
	char	nError	__attribute__ ((aligned (8))) = 1;
//	char* ERROR = "ERROR";

	// AT#CIND
	// AT+CIND? -> +CIND: 5,0,1,0,0,0,0,0,4 pos 3 from left '1' if connected to GSM service '0' otherwise
	// AT&V -> Display Current Base Configuration And Profile

	// Write AT+CIND command
		for( i = 0; i < 4; i++ )
		{
			if( gsmSendAtCommand( "+CIND?\r" ) )
//			if( writeStringToUxRTx( UART_GSM, "AT" ) )
//				if( waitForString( "AT", delay1s ) )
//					if( writeStringToGSM( "+CIND?\r" ) )
						break;

			/* Do not repeat do fast                         */
			/* Give module time to conenct to mobile network */
			DelayMs( 200 );
		}

	// Check CIND response
	//for( i = 0, j = 0; i < 15 && j < strlen( ERROR ); )
	//{
		if( waitForString( "+CIND: ", delay1s ) )
			for( j = 0; j < 15; j++ )
			{
				// Wait for character
				recUART( UART_GSM, &c );
				AtCind[j] = c;
				// Echo to UART_DEBUG
#ifdef __UART_DEBUG
				writeByteToUxRTx(UART_DEBUG, AtCind[j] );
#endif
			}

		else	// Got error message
		{
			memset( AtCind, 0 ,16 );
	    	nError = 0;
	 }
	//}

	return nError;
}

int gsm_wait_for_response(int gsm_response_code, unsigned int delay) {
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
	return waitForString((char*)gsm_responses[gsm_response_code][GSM_NUMERIC_RESPONSE], delay);
#else
	return waitForString((char*)gsm_responses[gsm_response_code][GSM_TEXT_RESPONSE], delay);
#endif
}

int waitForString( char* string, unsigned int delay )
{
	int	i, j;
//	char	c	__attribute__ ((aligned (8))); //REMOVED
	BYTE	c	__attribute__ ((aligned (8))); //ADDED
	char	nError	__attribute__ ((aligned (8)))= 1;
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
    char	ERRORstring[]	__attribute__ ((aligned (8)))= "4\r";
#else
	char	ERRORstring[]	__attribute__ ((aligned (8)))= "ERROR";
#endif

	TRACE_CALL_ENTER_PARAM_FMT("%s", string);
//	TRACE_CALL_ENTER_PARAM_FMT("booya");
	//int delay1ms = 0x13880;		// 1ms
	//int delay1s = 0x34C4B40;	// 3s
	//int delay1S = 0x1312D00;

	//for( i = 0, j = 0; i < strlen( string ) && j < strlen( ERROR ) && (ReadCoreTimer() < 0x4C4B40) < delay1s; )
	//for( i = 0, j = 0; (i < strlen( string )) && (j < strlen( ERROR )); )
	for( i = 0, j = 0; ( (i < strlen( string )) && (j < strlen( ERRORstring )) ); )   // "-1" for ending null
	{
		//delay = delay / 4;
		//WriteTimer45( 0 );
		WriteCoreTimer(0);
		// Wait for character
		do
		{
			// HB
			/* Changed to DEBUG port for test of behaivour then getting ERROR string */
			if(comm_uart_is_received_data_available(UART_GSM))
				//if (UARTReceivedDataIsAvailable( UART_DEBUG ) )
				//	while( !UARTReceivedDataIsAvailable( UART_GSM ) && (ReadTimer23() < delay1s) );
			{
				//	receive = 1;
				goto waitSorStringOk;
			}
			//}while( ReadTimer45() < delay );
		}
		while(ReadCoreTimer() < delay);

		TRACE_CALL_EXIT_PARAM_FMT("Abn");
		// Did not receive character in time
		return 0;

		// Did receive character in time
		waitSorStringOk:

		// HB
		/* Changed to DEBUG port for test of behaivour then getting ERROR string */
		recUART(UART_GSM, &c);
		//recUART( UART_DEBUG, &c );
		// Echo to UART_DEBUG
#ifdef __UART_DEBUG
		writeByteToUxRTx(UART_DEBUG, c);
#endif

		// Check character if expected
		if(c == string[i])
		{ // Expected
			i = i + 1;
			nError = 1;
			// HB
			/* Removed next line to improve searching for ERROR string */
			/* Otherthise no reset of ERROR counter after received beginning of wanted string */
			//continue;
		}
		else // Not expected
			i = 0;

		// Check for error message
		if(c == ERRORstring[j])
		{
			j = j + 1;
			nError = 0;
			//if( j < strlen( ERROR ) )
			//	break;
		}
		else
		{
			j = 0;
			nError = 1;
		}

		// HB
		/* Added check if ERROR string is part of wanted string */
		if(j <= i)
		{
			j = 0;
			nError = 1;
		}
	}

	TRACE_CALL_EXIT_PARAM_INT(nError);
	return nError;
}

//TODO DY Remove this legacy function which checks checksum.
int gsm_download_file_http_get_legacy(struct connection_state_t* conn_state)
{
  struct http_request_t* http_request = &(conn_state->request);
  uint32_t address = conn_state->data_store.mem_addr;
  char* fileDirectory = http_request->fileDirectory;
  char* fileName = http_request->fileName;
  char* httpVersion = http_request->httpVersion;
  char* hostString = http_request->server;
	/* int address - Start write address for download*/

	/* Local defines */
//	#define semBufferMax	1024
	#define semBufferMax	1023
	int		semBuffer					= semBufferMax;	/* Number of unused bytes in buffer[] */
	char	semBurningInProgress		__attribute__ ((aligned (8)))		= 0;	/* '1' if burning in external FLASH memory is in porgress */
	char	semReachedChecksumInFile	__attribute__ ((aligned (8)))	= 0;
	int		i							= 0;	/* Reserved counter variable */
	int		j							= 0;	/* Reserved counter variable */
	int		n							= 0;	/* Reserved counter variable */
//	char buffer[semBuffer];				/* Received characters not yet pushed to external FLASH memory */
	char	buffer[semBuffer + 1]		__attribute__ ((aligned (8)));				/* Received characters not yet pushed to external FLASH memory */
	char	dropped						__attribute__ ((aligned (8)))	= 0;	/* '1' if received character could not be buffered */
//	char	c							__attribute__ ((aligned (8)));									/* Temporary character variable */ //REMOVED
	BYTE	c							__attribute__ ((aligned (8)));									/* Temporary character variable */ //ADDED
	char	end							__attribute__ ((aligned (8)))	= 0;
//	char nError						= 1;	/* Indicates if no errors have occured */
	char	sectorEraseBegun			__attribute__ ((aligned (8)))	= 0;	/* Indicates whether An erase of sector have begun */
	unsigned int dataLength			= 0;	/* Number of byte written to external FLASH memory */
	unsigned int calculatedChecksum	= 0;
	unsigned int downloadedChecksum	= 0;
	unsigned int gsmStatusFake		= 0;
	char AtCindFake[22]					__attribute__ ((aligned (32)));
	char IMEIFake[16]					__attribute__ ((aligned (8)));
	char IMSIFake[16]					__attribute__ ((aligned (8)));
	unsigned int orgAddress		= address;
	int		characterReceived;

	clrUARTRxBuffer( UART_DEBUG );

	/* Prepare for first write */
	SST25CSHigh();
	//SST25SectorErase( SST25_CMD_SER04, address );
	//SST25SectorErase( SST25_CMD_SER04, 0x0 );
	//SST25PrepareWriteAtAddress( address );
	//SST25PrepareWriteAtAddress( 0x0 );

	/* Make sure the first sector has been erased */
	/* In case address is not even sector addres  */
	SST25SectorErase( SST25_CMD_SER04, address & ~sectorSize );
	SST25PrepareWriteAtAddress( address );


#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx( UART_DEBUG, "\r\norgAddress: " );
	puthex32(orgAddress);
	writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx(UART_DEBUG, "\r\nStart downloading file");
	writeStringToUxRTx(UART_DEBUG, "\r\nSending HTTP/GET\r\n");
	writeStringToUxRTx(UART_DEBUG, "GET ");
	writeStringToUxRTx(UART_DEBUG, fileDirectory);
	writeStringToUxRTx(UART_DEBUG, fileName);
	writeStringToUxRTx(UART_DEBUG, " HTTP/");
	writeStringToUxRTx(UART_DEBUG, httpVersion);
	writeStringToUxRTx(UART_DEBUG, " \r\nHost: ");
	writeStringToUxRTx(UART_DEBUG, hostString);
	writeStringToUxRTx(UART_DEBUG, "\r\n\r\n");
#endif

	writeStringToUxRTx(UART_GSM, "GET ");
	writeStringToUxRTx(UART_GSM, fileDirectory);
	writeStringToUxRTx(UART_GSM, fileName);
	writeStringToUxRTx(UART_GSM, " HTTP/");
	writeStringToUxRTx(UART_GSM, httpVersion);
	writeStringToUxRTx(UART_GSM, " \r\nHost: ");
	writeStringToUxRTx(UART_GSM, hostString);
	writeStringToUxRTx(UART_GSM, "\r\n\r\n");

	clrUARTRxBuffer( UART_GSM );

	/* Check if file exist */
	/* Parse HTTP headder  */
	if( waitForString( "HTTP/1.", delay50s ) )  // HTTP1.x.
		if( recUART( UART_GSM, &c ) )
			if( waitForString( " 200", delay10ms ) )    // Wait for status code 200.
				goto receiveFile;

	/* If file does not exist */
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
	waitForString( "3\r", delay30s );
#else
	waitForString( "NO CARRIER", delay30s );
#endif
	return 0;

	receiveFile:

	/* Wait for content/file body */
//	if( !waitForString( "\r\n\r\n", delay10s ) )     // Wait for start of body.
	for( i = 0, j = 0, n = 0; (i < 1048) && (n < 2); i++)
	{
		characterReceived = 0;
		WriteCoreTimer(0);
		// Wait for character
		do
		{
			// HB
			/* Changed to DEBUG port for test of behainvour then getting ERROR string */
			if(comm_uart_is_received_data_available(UART_GSM))
			{
				characterReceived = 1;
				break;
			}
		}
		while(ReadCoreTimer() < delay10s);
		if( !characterReceived )
			break;	// No character in UART_Rx register in time.

		recUART(UART_GSM, &c);

#ifdef __UART_DEBUG_DOWNLOAD
		// Echo to UART_DEBUG
		writeByteToUxRTx(UART_DEBUG, c);
#endif

		// Search for [\r\n\r\n] or [\n\n].
		if( c == '\n' )	// Did we receive a '\n'?
		{
			n++;
			j = 0;
		}
		else			// No we did not.
		{
			if( (c == '\r') && !j )
			{
				j++;
			}
			else
			{
				j = 0;
				n = 0;
			}


//			j++;
//
//			if( (c != '\r') && (j > 1) )
//			if( (c != '\r') || (j > 1) )
//			if( !((c == '\r') && (j < 2)) )
//			if( (c == '\r') && (j < 2) )
//			{
//				n++;
//			}
//			else
//			{
//				n = 0;
//			}
		}
	}
	i = 0;
	j = 0;
#ifdef __UART_DEBUG_DOWNLOAD
	writeByteToUxRTx(UART_DEBUG, '#');	// Just a marker in the ouput.
#endif
	if( n < 2 )
		return 0;

//	/* Check if file exist */
//	/* Parse HTTP headder  */
//	if( waitForString( "HTTP/1.", delay50s ) )
//		if( recUART( UART_GSM, &c ) )
//			if( waitForString( " 200", delay10ms ) )
//				goto receiveFile;
//
//	/* If file does not exist */
//	waitForString( "NO CARRIER", delay30s );
//	return 0;
//
//	receiveFile:
//	dataLength = 0;
//	/* Wait for content/file body */
//	if( !waitForString( "\r\n\r\n", delay3s ) )
//		return 0;

//	c = 0;	//qqq

        // Download body of file.
	WriteCoreTimer( 0 );
	while( 1 )
	{
		nextChar:

		/* Do not wait to long for a new character */
		if( ReadCoreTimer() > delay50s )
		{
#ifdef __UART_DEBUG_DOWNLOAD
			writeStringToUxRTx( UART_DEBUG, "\r\nTIMEOUT\r\n" );
#endif

			/* Turn off GSM module      */
			/* The hard way is required */
			gsmHardTurnOff( 5 );

			/* Restart GSM module */
			for( i = 0; i < 4; i++ )
				if( gsm_init( &gsmStatusFake, IMEIFake, IMSIFake, AtCindFake  ) )	/* Get International Mobile Equipment Identity from GSM module */
					break;
			//return 0;
			goto dropped;
		}

		if ( comm_uart_is_received_data_available( UART_GSM ) )
		//if( UARTReceivedDataIsAvailable( UART_DEBUG ) )
		{
	//					c = UARTGetDataByte( UART_GSM );
	//					writeByteToUxRTx( UART_DEBUG, ((c >> 4) < 0xA ? (c >> 4) + '0' : (c >> 4) - 0xA + 'A' ) );
	//					writeByteToUxRTx( UART_DEBUG, ((c & 0xF) < 0xA ? (c & 0xF) + '0' : (c & 0xF) - 0xA + 'A' ) );
	//					goto nextChar;

			/* Reset timer */
			WriteCoreTimer( 0 );

			if( semBuffer )
			{
				c = comm_uart_get_data_byte( UART_GSM );

				/* Check if we have reached to the point in file there the checksum is added */
				if( c == CHECKSUM_DELIMITER )
					semReachedChecksumInFile = 1;

				/* Calculate checksum of downloaded file */
				if(!semReachedChecksumInFile) {
					calculatedChecksum += (unsigned int)c;
				} else {
					if (c != END_OF_FILE && c != '\r' && c != '\n' && c != CHECKSUM_DELIMITER) {
						downloadedChecksum <<= 4;
						downloadedChecksum = downloadedChecksum + ((c < 'A') ? c - '0' : ((c < 'a') ? c - 'A' + 0xA : c - 'a' + 0xA ));
					}
				}

				/* Test for end of datastream */
        if (end == 0 && c == END_OF_FILE) {
          end = 8;
        } else if (end == 8) {
          end = 8;
        } else {
          end = 0;
        }
				buffer[i++] = c;
				semBuffer--;
				i &= semBufferMax;
			}
			else
			{
				dropped = 1;
				break;
			}
		}
		if( semBurningInProgress )
			semBurningInProgress = SST25IsWriteBusy();
		if( !semBurningInProgress )
		{
			if( semBuffer < semBufferMax )
			{
				/* Erase sector if needed */
				if( !(address & sectorSize) && !sectorEraseBegun )
				{
					SST25SectorErase( SST25_CMD_SER04, address );
					semBurningInProgress = 1;
					sectorEraseBegun = 1;

					goto nextChar;
				}
				else
					sectorEraseBegun = 0;
				/* Prepare start writing to new page */
				if( !(address & pageSize) )
				{
					SST25PrepareWriteAtAddress( address );
//					LED_SERVICE = ~LED_SERVICE;
					LED_SERVICE_INV;
				}
				/*if( !((address + lendata) & pageSize) )
					SST25PrepareWriteAtAddress( address + lendata );*/

		//		SPIPut( SPIPut( buffer[j++] ) );	// Move data to SST25

				SPIPut( buffer[j++] );
				semBuffer++;
//				j &= (semBufferMax -  1);
				j &= semBufferMax;

				/* Increment address to next byte address */
				dataLength++;	/* We have written another byte */
				address++;		/* Goto next address */

				/* Don not download more than can be kept in the memory region */
				if( address > ICPCODE_END )
					goto dropped;

				/* If write to last byte in page start the burning process */
				if( !(address & pageSize) )
				{
					while(!SpiChnIsSrEmpty(SPI_CHANNEL2));
					//for( skit = 0; skit < 1000000; skit++ );
					SST25CSHigh();
					semBurningInProgress = 1;
				}
			} else if (end > 7) {
					/* Start burning process */
					while(!SpiChnIsSrEmpty(SPI_CHANNEL2));
					SST25CSHigh();

					/* Wait for transmission to end */
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
					waitForString("3\r", delay30s);
#else
					waitForString( "NO CARRIER", delay30s );
#endif

					while( SST25IsWriteBusy() );

#ifdef __UART_DEBUG_DOWNLOAD
					writeStringToUxRTx(UART_DEBUG, "\r\nDownloaded checksum:   ");
					puthex32(downloadedChecksum);
					writeStringToUxRTx(UART_DEBUG, "\r\nCalculateded checksum: ");
					puthex32(calculatedChecksum);
					writeStringToUxRTx(UART_DEBUG, "\r\nAddress:               ");
					puthex32(address);
					writeStringToUxRTx(UART_DEBUG, "\r\nOrgiginal address:     ");
					puthex32(orgAddress);
					writeStringToUxRTx(UART_DEBUG, "\r\nData length: ");
					puthex32(dataLength);
					writeStringToUxRTx(UART_DEBUG, "\r\n");
#endif

					if( (downloadedChecksum == calculatedChecksum) && (address <= ICPCODE_END) )
					{
#ifdef __UART_DEBUG_DOWNLOAD
						writeStringToUxRTx( UART_DEBUG, "\r\nOK - CHECKSUM: while downloading file\r\n" );
#endif

						/* Calculate checksum of downloaded file in external FLASH-memory SST25 */
						SST25SetReadFromAddress( orgAddress );
						calculatedChecksum = 0;
						for( i = 0; i < dataLength-10; i++ )
						{
							c = SPIPut( 0 );
#ifdef __UART_DEBUG_DOWNLOAD_EXTRACT_CHARACTERS
						writeByteToUxRTx(UART_DEBUG, c);
#endif
							if( c == '$' )
								break;
							calculatedChecksum += c;
						}
						SST25CSHigh();

#ifdef __UART_DEBUG_DOWNLOAD
						writeStringToUxRTx(UART_DEBUG, "\r\nCalculateded checksum SST25: ");
						puthex32(calculatedChecksum);
						writeStringToUxRTx(UART_DEBUG, "\r\n");
#endif
						/* Insert downloading error for test */
						//calculatedChecksum++;

						/* Check if checksum is correct */
						if( calculatedChecksum == downloadedChecksum )
						{
							/* Tell bootloader that new ICP-file is downloaded */
							SST25PrepareWriteAtAddress( 0x0 );
							SPIPut( 0xA5 );
							while(!SpiChnIsSrEmpty(SPI_CHANNEL2));
							SST25CSHigh();

#ifdef __UART_DEBUG_DOWNLOAD
							writeStringToUxRTx( UART_DEBUG, "\r\nOK - CHECKSUM: in downloaded file \r\n" );
#endif
							WriteCoreTimer(0);
							while( SST25IsWriteBusy() );
							while( ReadCoreTimer() < delay250ms );  // Wait so all characters in above output on UART_DEBUG will be transmitted and saved.
							return 1;
						}
						else
						{
#ifdef __UART_DEBUG_DOWNLOAD
							writeStringToUxRTx( UART_DEBUG, "\r\nERROR - CHECKSUM: in downloaded file" );
#endif
							//dataLength = 0;
							return 0;
						}

//						#ifdef __UART_DEBUG_DOWNLOAD
//						writeStringToUxRTx( UART_DEBUG, "OK - CHECKSUM" );
//						#endif
//						DelayMs( 100 );
//						/* Tell bootloader that new ICP-file is downloaded */
//						SST25PrepareWriteAtAddress( 0x0 );
//						SPIPut( 0xA5 );
//						while(!SpiChnIsSrEmpty(SPI_CHANNEL2));
//						SST25CSHigh();
//						return 1;
					}
					else
					{
#ifdef __UART_DEBUG_DOWNLOAD
						writeStringToUxRTx( UART_DEBUG, "\r\nERROR - CHECKSUM: while downloading file\r\n" );
#endif
//						dataLength = 0;
						DelayMs( 100 );
						return 0;
					}
//					return dataLength;
				}
		}
	}


	dropped:
	if( dropped )
	{
#ifdef __UART_DEBUG_DOWNLOAD
		writeStringToUxRTx( UART_DEBUG, "\r\n!!! DROPPED !!!" );
#endif
//		i = 0;
//		while(1)
//		{
			WriteCoreTimer( 0 );
			// Wait for EndOfFile-character
			do
			{
				if (comm_uart_is_received_data_available( UART_GSM ) )
		    	{
//			    	i = 0;
			    	if( recUART( UART_GSM, &c ) )
			    		if( c == '.' )
		    				goto dropped_after;

			    	WriteCoreTimer( 0 );
		    	}
			}while( ReadCoreTimer() < delay3s );
			/* More than 10 tries without any received character */
			/* Give up                                           */
//			if( i++ > 9 )
//				break;
//		}
		dropped_after:
//#ifdef RESPONSE_FORMAT_RESULT_CODES
//              waitForString( "3\r", delay5s );
//#else
//		waitForString( "NO CARRIER", delay5s );
//#endif
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "3\r", delay30s );
#else
                waitForString( "NO CARRIER", delay30s );
#endif
		//nError = 0;
//		dataLength = 0;
		return 0;
		//return dataLength;
	}

//	gsmDownloadFileEnd:
	//return dataLength;
	return 0;
}

int gsm_download_file_http_get(struct connection_state_t* conn_state)
{
  struct http_request_t* http_request = &(conn_state->request);
  uint32_t address = conn_state->data_store.mem_addr;
  char* fileDirectory = http_request->fileDirectory;
  char* fileName = http_request->fileName;
  char* httpVersion = http_request->httpVersion;
  char* hostString = http_request->server;
	/* int address - Start write address for download*/

  TRACE_CALL_ENTER();

	/* Local defines */
//	#define semBufferMax	1024
	#define semBufferMax	1023
	int		semBuffer					= semBufferMax;	/* Number of unused bytes in buffer[] */
	char	semBurningInProgress		__attribute__ ((aligned (8)))		= 0;	/* '1' if burning in external FLASH memory is in porgress */
	char	semReachedCrcInFile	__attribute__ ((aligned (8)))	= 0;
	int		i							= 0;	/* Reserved counter variable */
	int		j							= 0;	/* Reserved counter variable */
	int		n							= 0;	/* Reserved counter variable */
//	char buffer[semBuffer];				/* Received characters not yet pushed to external FLASH memory */
	char	buffer[semBuffer + 1]		__attribute__ ((aligned (8)));				/* Received characters not yet pushed to external FLASH memory */
	char	dropped						__attribute__ ((aligned (8)))	= 0;	/* '1' if received character could not be buffered */
//	char	c							__attribute__ ((aligned (8)));									/* Temporary character variable */ //REMOVED
	BYTE	c							__attribute__ ((aligned (8)));									/* Temporary character variable */ //ADDED
	char	end							__attribute__ ((aligned (8)))	= 0;
//	char nError						= 1;	/* Indicates if no errors have occured */
	char	sectorEraseBegun			__attribute__ ((aligned (8)))	= 0;	/* Indicates whether An erase of sector have begun */
	unsigned int dataLength			= 0;	/* Number of byte written to external FLASH memory */
//	unsigned int calculatedChecksum	= 0;
	unsigned int downloaded_crc	= 0;
	unsigned int gsmStatusFake		= 0;
	char AtCindFake[22]					__attribute__ ((aligned (32)));
	char IMEIFake[16]					__attribute__ ((aligned (8)));
	char IMSIFake[16]					__attribute__ ((aligned (8)));
	unsigned int orgAddress		= address;
	int		characterReceived;

	clrUARTRxBuffer( UART_DEBUG );

	/* Prepare for first write */
	SST25CSHigh();
	//SST25SectorErase( SST25_CMD_SER04, address );
	//SST25SectorErase( SST25_CMD_SER04, 0x0 );
	//SST25PrepareWriteAtAddress( address );
	//SST25PrepareWriteAtAddress( 0x0 );

	/* Make sure the first sector has been erased */
	/* In case address is not even sector addres  */
	SST25SectorErase( SST25_CMD_SER04, address & ~sectorSize );
	SST25PrepareWriteAtAddress( address );


#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx( UART_DEBUG, "\r\norgAddress: " );
	puthex32(orgAddress);
	writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx(UART_DEBUG, "\r\nStart downloading file");
	writeStringToUxRTx(UART_DEBUG, "\r\nSending HTTP/GET\r\n");
	writeStringToUxRTx(UART_DEBUG, "GET ");
	writeStringToUxRTx(UART_DEBUG, fileDirectory);
	writeStringToUxRTx(UART_DEBUG, fileName);
	writeStringToUxRTx(UART_DEBUG, " HTTP/");
	writeStringToUxRTx(UART_DEBUG, httpVersion);
	writeStringToUxRTx(UART_DEBUG, " \r\nHost: ");
	writeStringToUxRTx(UART_DEBUG, hostString);
	writeStringToUxRTx(UART_DEBUG, "\r\n\r\n");
#endif

	writeStringToUxRTx(UART_GSM, "GET ");
	writeStringToUxRTx(UART_GSM, fileDirectory);
	writeStringToUxRTx(UART_GSM, fileName);
	writeStringToUxRTx(UART_GSM, " HTTP/");
	writeStringToUxRTx(UART_GSM, httpVersion);
	writeStringToUxRTx(UART_GSM, " \r\nHost: ");
	writeStringToUxRTx(UART_GSM, hostString);
	writeStringToUxRTx(UART_GSM, "\r\n\r\n");

	clrUARTRxBuffer( UART_GSM );

	/* Check if file exist */
	/* Parse HTTP headder  */
	if( waitForString( "HTTP/1.", delay50s ) )  // HTTP1.x.
		if( recUART( UART_GSM, &c ) )
			if( waitForString( " 200", delay10ms ) )    // Wait for status code 200.
				goto receiveFile;

	/* If file does not exist */
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
	waitForString( "3\r", delay30s );
#else
	waitForString( "NO CARRIER", delay30s );
#endif
	return 0;

	receiveFile:

	/* Wait for content/file body */
//	if( !waitForString( "\r\n\r\n", delay10s ) )     // Wait for start of body.
	for( i = 0, j = 0, n = 0; (i < 1048) && (n < 2); i++)
	{
		characterReceived = 0;
		WriteCoreTimer(0);
		// Wait for character
		do
		{
			// HB
			/* Changed to DEBUG port for test of behainvour then getting ERROR string */
			if(comm_uart_is_received_data_available(UART_GSM))
			{
				characterReceived = 1;
				break;
			}
		}
		while(ReadCoreTimer() < delay10s);
		if( !characterReceived )
			break;	// No character in UART_Rx register in time.

		recUART(UART_GSM, &c);

#ifdef __UART_DEBUG_DOWNLOAD
		// Echo to UART_DEBUG
		writeByteToUxRTx(UART_DEBUG, c);
#endif

		// Search for [\r\n\r\n] or [\n\n].
		if( c == '\n' )	// Did we receive a '\n'?
		{
			n++;
			j = 0;
		}
		else			// No we did not.
		{
			if( (c == '\r') && !j )
			{
				j++;
			}
			else
			{
				j = 0;
				n = 0;
			}
		}
	}
	i = 0;
	j = 0;
#ifdef __UART_DEBUG_DOWNLOAD
	writeByteToUxRTx(UART_DEBUG, '#');	// Just a marker in the ouput.
#endif
	if( n < 2 )
		return 0;

//	/* Check if file exist */
//	/* Parse HTTP headder  */
//	if( waitForString( "HTTP/1.", delay50s ) )
//		if( recUART( UART_GSM, &c ) )
//			if( waitForString( " 200", delay10ms ) )
//				goto receiveFile;
//
//	/* If file does not exist */
//	waitForString( "NO CARRIER", delay30s );
//	return 0;
//
//	receiveFile:
//	dataLength = 0;
//	/* Wait for content/file body */
//	if( !waitForString( "\r\n\r\n", delay3s ) )
//		return 0;

//	c = 0;	//qqq

        // Download body of file.
	WriteCoreTimer( 0 );
	while( 1 )
	{
		nextChar:

		/* Do not wait to long for a new character */
		if( ReadCoreTimer() > delay50s )
		{	
#ifdef __UART_DEBUG_DOWNLOAD
			writeStringToUxRTx( UART_DEBUG, "\r\nTIMEOUT\r\n" );
#endif

			/* Turn off GSM module      */
			/* The hard way is required */
			gsmHardTurnOff( 5 );

			/* Restart GSM module */
			for( i = 0; i < 4; i++ )
				if( gsm_init( &gsmStatusFake, IMEIFake, IMSIFake, AtCindFake  ) )	/* Get International Mobile Equipment Identity from GSM module */
					break;
			//return 0;
			goto dropped;
		}

		if ( comm_uart_is_received_data_available( UART_GSM ) )
		{
			/* Reset timer */
			WriteCoreTimer( 0 );

			if( semBuffer )
			{
				c = comm_uart_get_data_byte( UART_GSM );

				/* Check if we have reached to the point in file there the checksum is added */
				if (c == CHECKSUM_DELIMITER) {
					semReachedCrcInFile = 1;
				}

				//DY
//				/* Calculate checksum of downloaded file */
//				if(!semReachedChecksumInFile) {
//					calculatedChecksum += (unsigned int)c;
//				} else
				{
					if (c != END_OF_FILE && c != '\r' && c != '\n' && c != CHECKSUM_DELIMITER) {
						downloaded_crc <<= 4;
						downloaded_crc = downloaded_crc + ((c < 'A') ? c - '0' : ((c < 'a') ? c - 'A' + 0xA : c - 'a' + 0xA ));
					}	
				}

				/* Test for end of datastream */
        if (end == 0 && c == END_OF_FILE) {
          end = 8;
        } else if (end != 8) {
          end = 0;
        }

        //DY No need to save the incoming CRC in the flash as well.
        if (!semReachedCrcInFile) {
          buffer[i++] = c;
          semBuffer--;
          i &= semBufferMax;
        }
			}
			else
			{
				dropped = 1;
				break;
			}	
		}
		if (semBurningInProgress) {
			semBurningInProgress = SST25IsWriteBusy();
		}

		if (!semBurningInProgress) {
			if ( semBuffer < semBufferMax )
			{
				/* Erase sector if needed */
				if (!(address & sectorSize) && !sectorEraseBegun)
				{
					SST25SectorErase( SST25_CMD_SER04, address );
					semBurningInProgress = 1;
					sectorEraseBegun = 1;

					goto nextChar;
				}
				else
					sectorEraseBegun = 0;
				/* Prepare start writing to new page */
				if( !(address & pageSize) )
				{
					SST25PrepareWriteAtAddress( address );
//					LED_SERVICE = ~LED_SERVICE;
					LED_SERVICE_INV;
				}	
				/*if( !((address + lendata) & pageSize) )
					SST25PrepareWriteAtAddress( address + lendata );*/

		//		SPIPut( SPIPut( buffer[j++] ) );	// Move data to SST25

				SPIPut( buffer[j++] );
				semBuffer++;
//				j &= (semBufferMax -  1);
				j &= semBufferMax;

				/* Increment address to next byte address */
				dataLength++;	/* We have written another byte */
				address++;		/* Goto next address */

				/* Don not download more than can be kept in the memory region */
				if( address > ICPCODE_END )
					goto dropped;

				/* If write to last byte in page start the burning process */
				if( !(address & pageSize) )
				{
					while(!SpiChnIsSrEmpty(SPI_CHANNEL2));
					//for( skit = 0; skit < 1000000; skit++ );
					SST25CSHigh();
					semBurningInProgress = 1;
				}
			} else if (end > 7) {
					/* Start burning process */
					while(!SpiChnIsSrEmpty(SPI_CHANNEL2));
					SST25CSHigh();

					/* Wait for transmission to end */
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
					waitForString("3\r", delay30s);
#else
					waitForString( "NO CARRIER", delay30s );
#endif

					while( SST25IsWriteBusy() );

#ifdef __UART_DEBUG_DOWNLOAD
					writeStringToUxRTx(UART_DEBUG, "\r\nDownloaded crc:   ");
					puthex32(downloaded_crc);
//					writeStringToUxRTx(UART_DEBUG, "\r\nCalculateded checksum: ");
//					puthex32(calculatedChecksum);
					writeStringToUxRTx(UART_DEBUG, "\r\nAddress:               ");
					puthex32(address);
					writeStringToUxRTx(UART_DEBUG, "\r\nOriginal address:     ");
					puthex32(orgAddress);
					writeStringToUxRTx(UART_DEBUG, "\r\nData length: ");
					puthex32(dataLength);
					writeStringToUxRTx(UART_DEBUG, "\r\n");
#endif

					//DY
//					if ((downloaded_crc == calculatedChecksum) && (address <= ICPCODE_END))
					if (address <= ICPCODE_END)
					{
#ifdef __UART_DEBUG_DOWNLOAD
						writeStringToUxRTx( UART_DEBUG, "\r\nOK - Checking CRC \r\n" );
#endif

						//DY
//						/* Calculate checksum of downloaded file in external FLASH-memory SST25 */
//						SST25SetReadFromAddress( orgAddress );
//						calculatedChecksum = 0;
//						for( i = 0; i < dataLength-10; i++ )
//						{
//							c = SPIPut( 0 );
//#ifdef __UART_DEBUG_DOWNLOAD_EXTRACT_CHARACTERS
//						writeByteToUxRTx(UART_DEBUG, c);
//#endif
//							if( c == '$' )
//								break;
//							calculatedChecksum += c;
//						}

						SST25CSHigh();

//#ifdef __UART_DEBUG_DOWNLOAD
//						writeStringToUxRTx(UART_DEBUG, "\r\nCalculateded checksum SST25: ");
//						puthex32(calculatedChecksum);
//						writeStringToUxRTx(UART_DEBUG, "\r\n");
//#endif
//						/* Insert downloading error for test */
//						//calculatedChecksum++;

						/* Check if checksum is correct */
//						if( calculatedChecksum == downloaded_crc )
						if (check_crc_32_on_flash(orgAddress, dataLength, downloaded_crc))
						{
							/* Tell bootloader that new ICP-file is downloaded */
							SST25PrepareWriteAtAddress( 0x0 );
							SPIPut( 0xA5 );
							while(!SpiChnIsSrEmpty(SPI_CHANNEL2));
							SST25CSHigh();

#ifdef __UART_DEBUG_DOWNLOAD
							writeStringToUxRTx( UART_DEBUG, "\r\nOK - CRC: in downloaded file \r\n" );
#endif
							WriteCoreTimer(0);
							while( SST25IsWriteBusy() );
							while( ReadCoreTimer() < delay250ms );  // Wait so all characters in above output on UART_DEBUG will be transmitted and saved.
							return 1;
						}
						else
						{
#ifdef __UART_DEBUG_DOWNLOAD
							writeStringToUxRTx( UART_DEBUG, "\r\nERROR - CRC: in downloaded file" );
#endif
							//dataLength = 0;
							return 0;
						}
					}
					else	
					{
#ifdef __UART_DEBUG_DOWNLOAD
						writeStringToUxRTx( UART_DEBUG, "\r\nERROR - CHECKSUM: while downloading file\r\n" );
#endif
						DelayMs( 100 );
						return 0;
					}
				}
		}
	}	


	dropped:
	if( dropped )
	{
#ifdef __UART_DEBUG_DOWNLOAD
		writeStringToUxRTx( UART_DEBUG, "\r\n!!! DROPPED !!!" );
#endif
//		i = 0;
//		while(1)
//		{
			WriteCoreTimer( 0 );
			// Wait for EndOfFile-character
			do
			{
				if (comm_uart_is_received_data_available( UART_GSM ) )
		    	{
//			    	i = 0;
			    	if( recUART( UART_GSM, &c ) )
			    		if( c == '.' )
		    				goto dropped_after;

			    	WriteCoreTimer( 0 );
		    	}
			}while( ReadCoreTimer() < delay3s );
			/* More than 10 tries without any received character */
			/* Give up                                           */
//			if( i++ > 9 )
//				break;
//		}
		dropped_after:
//#ifdef RESPONSE_FORMAT_RESULT_CODES
//              waitForString( "3\r", delay5s );
//#else
//		waitForString( "NO CARRIER", delay5s );
//#endif
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "3\r", delay30s );
#else
                waitForString( "NO CARRIER", delay30s );
#endif
		//nError = 0;
//		dataLength = 0;
		return 0;
		//return dataLength;
	}

//	gsmDownloadFileEnd:
	//return dataLength;
	return 0;
}

void writeInt( int iint )
{
#ifdef __UART_DEBUG
	int				i;
	unsigned char	c	__attribute__ ((aligned (8)));

	for( i = 7; i >= 0; i = i - 1 )
	{
		c = iint >> ( 4 * i );
		c &= 0xF;
		if( c < 0xA )
			writeByteToUxRTx( UART_DEBUG, c + 0x30 );
		else
			writeByteToUxRTx( UART_DEBUG, c + 'A' - 0xA );
	}
#else
	;
#endif
}


int gsmSendAtCommand( char* atCommand )
{
	int i,j;
	int nError = 1;	/* '1' -> no error occured */

	//DY
//	TRACE_CALL_ENTER_PARAM_FMT(atCommand);
	TRACE_CALL_ENTER_PARAM_FMT("%s", atCommand);

	for( i = 0; i < 4; i = i + 1 )
	{
		/* Clear GSM modules RS-232 indata buffert */
		for ( j = 0; j < 4; j = j + 1 )
		{
			int length = 16;
			BYTE tmp[length];

			// Clear string.
			memset(&tmp,0,sizeof(tmp));

			/* CLear UART_GSM receive buffer */
			clrUARTRxBuffer( UART_GSM );

      if (writeStringToUxRTx( UART_GSM, "AT\r")) {
        // Receive answer from GSM module.
        if (gsmWaitForOk(tmp, length, delay1s)) {
          break;
        }
      }
		}
		/* Check if successful */
//		if( j > 3 )
//			continue;

		if (j < 4) {
      /* CLear UART_GSM receive buffer */
      DelayMs(100);
      clrUARTRxBuffer( UART_GSM);

      /* Send actual AT-command */
      if (writeStringToUxRTx( UART_GSM, "AT")) {
        if (writeStringToGSM(atCommand)) {
          break;
        }
      }
    }
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nSendAtCommand one more turn\r\n" );
#endif
	}
	if( i > 3 )
		nError = 0;

	TRACE_CALL_ENTER_PARAM_INT(nError);
	return nError;
}

int gsmSetEmailConfig( void )
{
	int nError = 1;

        // Set SMTP-server address.
        gsmSendAtCommand( EMAIL_SERVER_COMMAND );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        if( !waitForString( "0\r", delay1s ) )
#else
	if( !waitForString( "OK\r\n", delay1s ) )
#endif
		nError = 0;

        // Set SMTP user name.
        gsmSendAtCommand( EMAIL_USER_COMMAND );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        if( !waitForString( "0\r", delay1s ) )
#else
	if( !waitForString( "OK\r\n", delay1s ) )
#endif
		nError = 0;

        // Set SMTP user password.
        gsmSendAtCommand( EMAIL_PASSWORD_COMMAND );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        if( !waitForString( "0\r", delay1s ) )
#else
	if( !waitForString( "OK\r\n", delay1s ) )
#endif
		nError = 0;

        // Set e-mail sender address.
	gsmSendAtCommand( EMAIL_SENDER_ADDRESS_COMMAND );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        if( !waitForString( "0\r", delay1s ) )
#else
	if( !waitForString( "OK\r\n", delay1s ) )
#endif
		nError = 0;

        // Save e-mail parameters.
	gsmSendAtCommand( "#ESAV\r" );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        if( !waitForString( "0\r", delay1s ) )
#else
	if( !waitForString( "OK\r\n", delay1s ) )
#endif
		nError = 0;

	return nError;
}

void gsmCheckEmailConfig( void )
{
#ifdef __UART_DEBUG
	unsigned char c;
#endif

	// Set SMTP-server address.
	gsmSendAtCommand( "#ESMTP?\r" );
#ifdef __UART_DEBUG
	while(1)
	{
		if( recUART( UART_GSM, &c) )
			writeByteToUxRTx( UART_DEBUG, c );
		else
			break;
	}
#endif

	// Set SMTP user name.
	gsmSendAtCommand( "#EUSER?\r" );
#ifdef __UART_DEBUG
	while(1)
	{
		if( recUART( UART_GSM, &c) )
			writeByteToUxRTx( UART_DEBUG, c );
		else
			break;
	}
#endif

	// Set SMTP user password.
	// Can not be checked.

	// Set e-mail sender address.
	gsmSendAtCommand( "#EADDR?\r" );
#ifdef __UART_DEBUG
	while(1)
	{
		if( recUART( UART_GSM, &c) )
			writeByteToUxRTx( UART_DEBUG, c );
		else
			break;
	}
#endif
}

// This function outputs a character to both the GSM and the DEBUG port.
void putc_GSM_DEBUG(char character)
{
//	char c;  // Used to empty buffer.

	writeByteToUxRTx( UART_GSM, character );
#ifdef __UART_DEBUG
	writeByteToUxRTx( UART_DEBUG, character );
	//recUART( UART_GSM, &c );
	DelayMs(1);
#endif
}

// The following function is currently not used!
#ifdef GSM_EMAIL_STATUS
// AW defined pins
//int gsmEmailStatus( unsigned int* gsmStatus, char* IMEI, char* IMSI, unsigned int* externalFlashMemorySst25Status, unsigned int* ismStatus )
int gsmEmailStatus( unsigned int* gsmStatus, char* IMEI, char* IMSI, unsigned int* externalFlashMemorySst25Status, int* RF_T_Array, int* RF_RH_Array, char* RF_N_Array, char* AtCind, unsigned int* S0_pulses )
{
	int				i,i2;
	char			c	__attribute__ ((aligned (8)));
	unsigned int	value;
	int 			AD_T_Array[4]; // T_sup,T_ret,T_tank,T_box;

	gsmSendAtCommand( "+CMEE=2\r" );

	/* Check if we can send e-mail. */
	/* If not try to connect.       */
	for( i = 0; i < 10; i = i + 1 )
	{
		if( gsmGetConnectionToMobileNetworkStatus( AtCind ) )
		{
			if( AtCind[4] == '1' )
			{
				*gsmStatus = *gsmStatus | GSM_CONNECTED_TO_MOBILE_NETWORK;
				writeStringToGSM( "GSM module was connected to mobile network\r\n" );
				//for( i = 0; i < 2; i = i + 1 )
				if( gsmInitGprs( "1,\"IP\",\"public.telenor.se\",\"0.0.0.0\",0,0" ) )
				{
					DelayMs( 5000 );
					if( gsmEstablishGPRS() )
					{
						gsmSetEmailConfig();
						break;
					}
				}	
			}
			else
			{
				*gsmStatus = *gsmStatus & ~GSM_CONNECTED_TO_MOBILE_NETWORK;
				writeStringToGSM( "GSM module was NOT connected to mobile network\r\n" );
			}
		}

		/* Wait 2 seconds until next check */
		DelayMs(2000);
	}
	if( i > 9 )
	{
		#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "Could NOT establish GPRS connection\r\n" );
		#endif
		return 0;
	}

	#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nSending e-mail - AT#SEMAIL...\r\n" );
	#endif

	//if( gsmSendAtCommand( "#SEMAIL=\"henrik.borg@atcindu.com\",\"Node E-mail " ) )
	#ifdef	__DEBUG
	  #ifdef __AW
	  	if( gsmSendAtCommand( "#SEMAIL=\"anders.widgren@atcindu.com\",\"E-mail NodeID " ) )
	  #else
		//if( gsmSendAtCommand( "#SEMAIL=\"anders.widgren@atcindu.com\",\"E-mail NodeID " ) )
		// HB
		if( gsmSendAtCommand( "#SEMAIL=\"henrik.borg@atcindu.com\",\"E-mail NodeID " ) )
	  #endif
//		//if( gsmSendAtCommand( "#SEMAIL=\"anders.widgren@atcindu.com\",\"E-mail NodeID " ) )
//		// HB
//		if( gsmSendAtCommand( "#SEMAIL=\"henrik.borg@atcindu.com\",\"E-mail NodeID " ) )
	#else
		if( gsmSendAtCommand( "#SEMAIL=\"m2m@atcindu.com\",\"E-mail NodeID " ) )
	#endif //__DEBUG
	//writeStringToGSM( "#SEMAIL=\"henrik.borg@atcindu.com\",\"Node E-mail " );
		//				  "#semail=\"henrik.borg@atcindu.com\",\"48""
		//				  "#semail=\"henrik.borg@bredband.net\",\"
	//if( writeStringToGSM( "#SEMAIL=\"trued.holmquist@atcindu.com\",\"NodeID: " ) )
	{
		/* The if-statement below or the above if-statement�s function call does not work properly */
		//if( writeStringToGSM( IMEI ) )
		clrUARTRxBuffer( UART_GSM );
		writeStringToGSM( IMEI );
		{
			//if( writeStringToGSM( "\"\r" ) )
			writeStringToGSM( "\"\r" );		// End of command to send Email. AT#SEMAIL
			{
				if( waitForString( "> ", delay2s ))	/* After this string has received we kan type in our message body */
				{
					writeStringToGSM( "\r\nNode ID: " );
					writeStringToGSM( IMEI );
					writeStringToGSM( "\r\nSim card:s IMSI: " );
					writeStringToGSM( IMSI );
					writeStringToGSM( "\r\n\r\n" );

					// Get AD readings.
					SampleAD_Sensors(AD_T_Array);

					/* Test electric heating stearing */
					EL_OUT1=0;	// Make output low.
					EL_OUT2=0;	// Make output low.
					EL_OUT3=0;	// Make output low.

					// Toggle relay back and forth to show installer that sequence test is about to start:
					HVAC_EN=1; // Turn on the HVAC-interface. Fix 111212:
					DelayMs( 500 );

					HVAC_EN=0; // Turn on the HVAC-interface. Fix 111212:
					DelayMs( 500 );

					HVAC_EN=1; // Turn on the HVAC-interface. Fix 111212:
					DelayMs( 500 );

					HVAC_EN=0; // Turn on the HVAC-interface. Fix 111212:
					DelayMs( 500 );

					HVAC_EN=1; // Turn on the HVAC-interface. Fix 111212:
					DelayMs( 2000 );

					EL_OUT1=1;
					DelayMs( 2000 );

					EL_OUT2=1;
					DelayMs( 2000 );

					EL_OUT3=1;
					DelayMs( 2000 );

					writeStringToGSM( "Wired sensors:\r\n\r\n" );

					// Testing the SENSE_R function
					writeStringToGSM( "\tToutd\t=\t" );
					value=SampleR_Sense();

					if(value==SHORT_CIRCUIT_SENSE_R)
					{
						writeStringToGSM( "Shorted!" );
					}
					else if(value==OPEN_CIRCUIT_SENSE_R)
					{
						writeStringToGSM( "    -" );
					}
					else
					{
						putdec(value); // Output temperature in decimal, the 15 least significant bits are fractional part
						writeStringToGSM( " ohms" );
					}
					writeStringToGSM( "\r\n" );

					// ******************TEST OF S0*******************
					writeStringToGSM( "\tS0-in\t=\t" );
					putdec((unsigned int) *S0_pulses);
					writeStringToGSM( " pulses\r\n" );

					// ******************TEST OF AD*******************
					// Output results from AD-connected boiler sensors etc
					for( i2 = 0; i2 < 4; i2 = i2 + 1 )
					{
						int j;

						switch (i2)
						{
							case 0:		writeStringToGSM( "\tT_sup" ); break;
							case 1:		writeStringToGSM( "\tT_ret" ); break;
							case 2:		writeStringToGSM( "\tT_tap" ); break;
							case 3:		writeStringToGSM( "\tT_pcb" ); break;
						}
						writeStringToGSM( "\t=\t" );

						j = AD_T_Array[i2];		// Temperature

						if(j==SHORT_CIRCUIT_AD)
						{
							writeStringToGSM( "Shorted!" );
						}
						else if(j==OPEN_CIRCUIT_AD)
						{
							writeStringToGSM( "    -" );
						}
						else
						{
							putdec(j>>16);  // Output temperature in decimal, the 16 least significant bits are fractional part
							putc_GSM_DEBUG( '.' );

							// Print one "decimal decimal". There are 16 "binary decimals".
							// The 1st decimal = (frac part) * 10 / 2^(binary decimals +1)
							// The 1st decimal = (frac part) * 10 >> 17
							putc_GSM_DEBUG( ( ( (j & 0b1111111111111111 ) *10 ) >> 17 ) + '0' );
							writeStringToGSM( "C" );
 						}
 						writeStringToGSM( "\r\n" );
 					}

					writeStringToGSM( "\r\n" );

					// Output radio sensor status

					writeStringToGSM( "GSM and RADIO:\r\n\r\n" );

					// AtCind[16] Signal strength [0..5]
					putSignalStrengthBar( AtCind[16] - '0' );
					writeStringToGSM( "\tGSM\t=\t Telenor\r\n" );

					for( i2 = 0; i2 < 4; i2 = i2 + 1 )
					{
						char	n	__attribute__ ((aligned (8)));
						int		j;

						n = (char)RF_N_Array[i2];	// "Strength"
						putSignalStrengthBar(n);

						writeStringToGSM( "\tT_" );
						c = i2 + 0x30 + 1;   //Output channel 0..3 as T1..T4 = how channels are indicated on the thermostats.
						putc_GSM_DEBUG( c );
						writeStringToGSM( "\t=\t" );

						j = RF_T_Array[i2];		// Temperature

						if(n)
						{
 							// Output temperature in decimal, the 16 least significant bits are fractional part
							putdec(j>>16);

							putc_GSM_DEBUG( '.' );

							// Print one "decimal decimal". There are 15 "binary decimals".
							// The 1st decimal = (frac part) * 10 / 2^(binary decimals +1)
							// The 1st decimal = (frac part) * 10 >> 17
							putc_GSM_DEBUG( ( ( (j & 0b1111111111111111 ) *10 ) >> 17 ) + '0' );
							writeStringToGSM( "C" );
 						}
 						else
 						{
	 						// Formatting spaces
 							writeStringToGSM( "  -  " );
 						}
 						writeStringToGSM( "\r\n" );
					}

					// Output GSM test summary

					// Check if GSM module was proparly  configured
					#define initiated (GSM_ON | GSM_IPR_SET | GSM_FLOW_CONTROL_SET | GSM_COMMAND_LEVEL_SET |\
										GSM_ERROR_LEVEL_SET | GSM_TE_CHARACTER_SET |\
										GSM_TCP_REASSEMBLY_SET | GSM_USER_ID_SET | GSM_USER_PASSWORD_SET |\
										GSM_ICMP_SET | GSM_STORED_IN_PROFILE_0 | GSM_FIREWALL_INITIATED |\
										GSM_GPRS_INITIATED | GSM_IMEI | GSM_IMSI)
					if( (*gsmStatus & ( initiated ) ) == ( initiated ) )
						writeStringToGSM( "\r\nGSM module was proparly configured\r\n" );
					else
						writeStringToGSM( "\r\nGSM module was NOT proparly  configured\r\n" );

					if( *gsmStatus & GSM_CONNECTED_TO_MOBILE_NETWORK )
						writeStringToGSM( "GSM module was connected to mobile network\r\n" );
					else
						writeStringToGSM( "GSM module was NOT connected to mobile network\r\n" );

// Don't include all the details, the 1K limit of the email doesn't allow it!
#ifdef GSM_DETAILS
					#ifdef __UART_DEBUG
					writeStringToUxRTx( UART_DEBUG, "\r\nGSM details\r\n" );
					#endif
					for( i = 1; i < 0x3000000000000; i = 2 * i )	// Max value of unsigned int is 0x4000000000000
					{
						if( i > GSM_ATCIND_COMMAND_RUNNED )
							break;

						#define GSM_SWITCH_TEST(i) ((*gsmStatus & i) ? writeStringToGSM( "1 (1)  " ) : writeStringToGSM( "0 (1/0)" ))

						// HB
						/* Moved \r\n into below case-statements for better look */
						switch( i )
						{
							case GSM_INITIATED:						GSM_SWITCH_TEST(i);  writeStringToGSM( " INITIATED\r\n" ); break;
							case GSM_ON:							GSM_SWITCH_TEST(i);  writeStringToGSM( " ON\r\n" ); break;
							case GSM_IPR_SET:						GSM_SWITCH_TEST(i);  writeStringToGSM( " IPR_SET\r\n" ); break;
							case GSM_FLOW_CONTROL_SET:				GSM_SWITCH_TEST(i);  writeStringToGSM( " FLOW_CONTROL_SET\r\n" ); break;
							case GSM_COMMAND_LEVEL_SET:				GSM_SWITCH_TEST(i);  writeStringToGSM( " COMMAND_LEVEL_SET\r\n" ); break;
							case GSM_ERROR_LEVEL_SET:				GSM_SWITCH_TEST(i);  writeStringToGSM( " ERROR_LEVEL_SET\r\n" ); break;
							case GSM_TE_CHARACTER_SET:				GSM_SWITCH_TEST(i);  writeStringToGSM( " TE_CHARACTER_SET\r\n" ); break;
							case GSM_TCP_REASSEMBLY_SET:			GSM_SWITCH_TEST(i);  writeStringToGSM( " TCP_REASSEMBLY_SET\r\n" ); break;
							case GSM_USER_ID_SET:					GSM_SWITCH_TEST(i);  writeStringToGSM( " USER_ID_SET\r\n" ); break;
							case GSM_USER_PASSWORD_SET:				GSM_SWITCH_TEST(i);  writeStringToGSM( " USER_PASSWORD_SET\r\n" ); break;
							case GSM_ICMP_SET:						GSM_SWITCH_TEST(i);  writeStringToGSM( " ICMP_SET\r\n" ); break;
							case GSM_STORED_IN_PROFILE_0:			GSM_SWITCH_TEST(i);  writeStringToGSM( " STORED_IN_PROFILE_0\r\n" ); break;
							case GSM_STORED_IN_PROFILE_1:			GSM_SWITCH_TEST(i);  writeStringToGSM( " STORED_IN_PROFILE_1\r\n" ); break;
							case GSM_FIREWALL_INITIATED:			GSM_SWITCH_TEST(i);  writeStringToGSM( " FIREWALL_INITIATED\r\n" ); break;
							case GSM_CONNECTED_TO_MOBILE_NETWORK:	GSM_SWITCH_TEST(i);  writeStringToGSM( " CONN_TO_MOBILE_NETWORK\r\n" ); break;
							case GSM_GPRS_INITIATED:				GSM_SWITCH_TEST(i);  writeStringToGSM( " GPRS_INITIATED\r\n" ); break;
							case GSM_GPRS_ESTABLISHED:				GSM_SWITCH_TEST(i);  writeStringToGSM( " GPRS_ESTABLISHED\r\n" ); break;
							case GSM_CONNECTED_TO_SERVER:			GSM_SWITCH_TEST(i);  writeStringToGSM( " CONNECTED_TO_SERVER\r\n" ); break;
							case GSM_IMEI:							GSM_SWITCH_TEST(i);  writeStringToGSM( " IMEI\r\n" ); break;
							case GSM_IMSI:							GSM_SWITCH_TEST(i);  writeStringToGSM( " IMSI\r\n" ); break;
							case GSM_SOCKET_CLOSED:					GSM_SWITCH_TEST(i);  writeStringToGSM( " SOCKET_CLOSED\r\n" ); break;
							case GSM_GOT_HTTP:						GSM_SWITCH_TEST(i);  writeStringToGSM( " GOT_HTTP\r\n" ); break;
							case GSM_ATCIND_COMMAND_RUNNED:			GSM_SWITCH_TEST(i);  writeStringToGSM( " ATCIND_COMMAND_RUNNED\r\n" ); break;
						}
						//gsmStatus >> (31 - i)) & 0x1
						//if( *gsmStatus & ( 1 << i ) )
						/*if( (*gsmStatus >> i) & 0x1 )
							writeByteToUxRTx( UART_DEBUG, '1' );
						else
							writeByteToUxRTx( UART_DEBUG, '0' );*/
						//( (*gsmStatus & i) ? writeStringToGSM( "1" ) : writeStringToGSM( "0" ));
						//( (gsmStatus >> (31 - i)) & 0x1 ? writeByteToUxRTx( UART_DEBUG, '1' ) : writeByteToUxRTx( UART_DEBUG, '0' ));

						// HB
						/* Moved into above case-statements for better look */
						//writeStringToGSM( "\r\n" );
					}
#endif	// GSM_DETAILS
					writeStringToGSM( "\r\nGSM status string:\r\n" );
					for( i = 0; i < 32; i++ )
						( (*gsmStatus >> (31 - i)) & 0x1 ? writeStringToGSM( "1" ) : writeStringToGSM( "0" ));
					writeStringToGSM( "\r\n" );

					/* External FLASH memory SST25 */
					if( *externalFlashMemorySst25Status )
						writeStringToUxRTx( UART_GSM, "Ex-FLASH OK" );
					else
						writeStringToUxRTx( UART_GSM, "Ex-FLASH NOT OK" );
					writeStringToGSM( "\r\n\r\n" );

					// **************************** Tests completed ***********************************

					HVAC_EN=0; // Turn off the HVAC-interface.
					EL_OUT1=0;	// Make output low.
					EL_OUT2=0;	// Make output low.
					EL_OUT3=0;	// Make output low.

					/* Start sending E-mail by issuing command Ctrl+Z */
					writeByteToUxRTx( UART_GSM, 0x1A );

					//waitForString( "OK\r\n", delay30s );
					/*if( waitForString( "OK\r\n", delay1s ))
						writeStringToUxRTx( UART_DEBUG, "E-mail successfully send\r\n" );
					else
						writeStringToUxRTx( UART_DEBUG, "E-mail NOT send\r\n" );*/
				}
			}
		}
	}
#ifdef RESPONSE_FORMAT_RESULT_CODES
        if( waitForString( "0\r", delay50s ) )
#else
	if( waitForString( "OK\r\n", delay50s ) )
#endif
		return 1;
	else
		return 0;
}
#endif

int gsmEmailString( char* emailAddress, char* stringToSend, char* IMEI, char* IMSI, char* AtCind )
{
	int i;

#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nSending e-mail status of firmware upgrade\r\n" );
#endif

	for( i = 0; i < 10; i = i + 1 )
	{
		if( gsmGetConnectionToMobileNetworkStatus( AtCind ) )
		{
			if( AtCind[4] == '1' )
			{
//				*gsmStatus = *gsmStatus | GSM_CONNECTED_TO_MOBILE_NETWORK;
//				writeStringToGSM( "GSM module was connected to mobile network\r\n" );
				//for( i = 0; i < 2; i = i + 1 )
//				if( gsmInitGprs( "1,\"IP\",\"public.telenor.se\",\"0.0.0.0\",0,0" ) )
//                                if( gsmEstablishGPRS() )
				{
					DelayMs( 5000 );
					if( gsmEstablishGPRS() )
					{
						gsmSetEmailConfig();
						break;
					}
				}
			}
//			else
//			{
//				*gsmStatus = *gsmStatus & ~GSM_CONNECTED_TO_MOBILE_NETWORK;
//				writeStringToGSM( "GSM module was NOT connected to mobile network\r\n" );
//			}
		}

		/* Wait 2 seconds until next check */
		DelayMs(2000);
	}
	if( i > 9 )
	{
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "Could NOT establish GPRS connection\r\n" );
#endif
		return 0;
	}

	//writeStringToUxRTx( UART_GSM, 'AT\r' );
	//waitForString( "AT\r", delay3s );
	//waitForString( "OK\r\n", delay3s );

	//for( i = 0; i < 4; i = i + 1 )
	//{
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nSending e-mail - AT#SEMAIL...\r\n" );
#endif
	//writeByteToUxRTx( UART_GSM, 'A' );
	//writeByteToUxRTx( UART_GSM, 'T' );
	//if( waitForString( "AT", delay1s ) )
	//	break;
	//}
	//if( i > 3 )
	//	return 0;
	clrUARTRxBuffer( UART_GSM );

	//if( gsmSendAtCommand( "#SEMAIL=\"henrik.borg@atcindu.com\",\"Node E-mail " ) )
#ifdef	__DEBUG
	  #ifdef __AW
	  	if( gsmSendAtCommand( "#SEMAIL=\"anders.widgren@atcindu.com\",\"E-mail NodeID " ) )
	  #else
		//if( gsmSendAtCommand( "#SEMAIL=\"anders.widgren@atcindu.com\",\"E-mail NodeID " ) )
		// HB
		if( gsmSendAtCommand( "#SEMAIL=\"henrik.borg@atcindu.com\",\"E-mail NodeID " ) )
	  #endif
#else
//		if( gsmSendAtCommand( "#SEMAIL=\"m2m@atcindu.com\",\"E-mail NodeID " ) )
//            if( gsmSendAtCommand( "\r" ) )
//                if( waitForString( "0\r", delay100ms ) )
//                if( writeStringToUxRTx( UART_GSM, "#SEMAIL=\"m2m@atcindu.com\",\"E-mail NodeID " ) )
//                writeStringToUxRTx( UART_DEBUG, "\r\nE-mailaddress: " );
//                writeStringToUxRTx( UART_DEBUG, emailAddress );
//                writeStringToUxRTx( UART_DEBUG, "\r\n" );

        // This should be able to work.
//        if( gsmSendAtCommand( "#SEMAIL=\"" ) )
//                if( writeStringToGSM( emailAddress ) )
//                        if( writeStringToGSM( "\",\"E-mail NodeID " ) )

//                                if( gsmSendAtCommand( "#SEMAIL=\"m2m@atcindu.com\",\"E-mail NodeID " )
//                                if( gsmSendAtCommand( "#SEMAIL=\"m2m@atcindu.com\",\"E-mail NodeID " ) )
//                                if( gsmSendAtCommand( "#semail=\"m2m@atcindu.com\",\"E-mail NodeID 123456789012345\"\r" ) )//henrik.borg@atcindu.com
//                                    if( waitForString( "> ", delay2s ))
//                                    {
//                                        writeStringToGSM( "Testar detta manuellt m2m 4" );
//                                        writeByteToUxRTx( UART_GSM, 0x1A );
//                                    }
        if( gsmSendAtCommand( "#semail=\"m2m@atcindu.com\",\"E-mail NodeID " ) )//henrik.borg@atcindu.com
	#endif //__DEBUG
	//writeStringToGSM( "#SEMAIL=\"henrik.borg@atcindu.com\",\"Node E-mail " );
		//				  "#semail=\"henrik.borg@atcindu.com\",\"48""
		//				  "#semail=\"henrik.borg@bredband.net\",\"
	//if( writeStringToGSM( "#SEMAIL=\"trued.holmquist@atcindu.com\",\"NodeID: " ) )
	{
		/* The if-statement below or the above if-statement�s function call does not work properly */
		//if( writeStringToGSM( IMEI ) )
		//clrUARTRxBuffer( UART_GSM );
		writeStringToGSM( IMEI );
		{
			//if( writeStringToGSM( "\"\r" ) )
			writeStringToGSM( "\"\r" );		// End of command to send Email. AT#SEMAIL
			{
				if( waitForString( "> ", delay2s ))	/* After this string has received we kan type in our message body */
				{
//                                    writeStringToGSM( "Testar detta manuellt m2m 8" );
					writeStringToGSM( "\r\nNode ID: " );
//					writeStringToGSM( IMEI );
//                                        writeStringToUxRTx( UART_GSM, "\r\nIMEI " );
                                        writeStringToUxRTx( UART_GSM, IMEI );
//                                        writeStringToUxRTx( UART_GSM, "\r\n" );
					writeStringToGSM( "\r\nSim card:s IMSI: " );
//					writeStringToGSM( IMSI );
//                                        writeStringToUxRTx( UART_GSM, "\r\nIMSI " );
                                        writeStringToUxRTx( UART_GSM, IMSI );
//                                        writeStringToUxRTx( UART_GSM, "\r\n" );
					writeStringToGSM( "\r\n\r\n" );
//					writeStringToGSM( stringToSend );
//                                        writeStringToUxRTx( UART_GSM, "\r\nString to send " );
                                        writeStringToUxRTx( UART_GSM, stringToSend );
//                                        writeStringToUxRTx( UART_GSM, "\r\n" );
					/* Start sending E-mail by issuing command Ctrl+Z */
					writeByteToUxRTx( UART_GSM, 0x1A );
				}
			}
		}
	}
#ifdef	__UART_DEBUG
        writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        if( waitForString( "0\r", delay50s ) )
#else
	if( waitForString( "OK\r\n", delay50s ) )
#endif
        {
#ifdef	__UART_DEBUG
            writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif
            return 1;
        }
	else
        {
#ifdef	__UART_DEBUG
            writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif
            return 0;
        }
}


//TODO DY Nobody calls this function so it should be removed.
//void gsmDownloadFileHttpGetMain( unsigned int* gsmStatus, char* IMEI, char* IMSI, char* AtCind )
//{
//	int				i;
////	unsigned char 	c;
////	int 			counter;
//	int				downloadFromServer;	/* We number will tell us which server do download from */
//
//	/* Used during trying to download new firmware */
//	char			retries			__attribute__ ((aligned (8)))	= 0;
//	char			totalRetries	__attribute__ ((aligned (8)))	= 0;
//
//	/* Try to download new firmware */
//	totalRetries = 0;
//	// HB
//	/* Temporary set to 1 */
//	//downloadFromServer = 0;
//	downloadFromServer = 1;
//	while(1)
//	{
//		// HB
//		/* Temporary uncomment delay */
//		/* restartDownloadingTry: */
//		//for( i = 0; i < 100; i = i + 1 )	/* 50 minutes */
//		//for( i = 0; i < 2; i = i + 1 )	/* 1 minutes 40 seconds  */
//		//	DelayMs(delay50s);
//
//		if( totalRetries < 10 )
//		{
//			/* Connect to server   */
//			/* If GPRS established */
//			//if( gsmGprsEstablished( gsmStatus ) )
//			if( gsmEstablishGPRS() )
//			{
//#ifdef __UART_DEBUG
//				writeStringToUxRTx( UART_DEBUG, "\r\nGPRS Established\r\n" );
//#endif
//				/* the node's id is the name of the file to download */
//				gsmGetImei( IMEI );
//
//				for( retries = 0; retries < 6; retries++ )
//				{
//					if( gsmConnectToServer_Multi( downloadFromServer ) )
//					{
//#ifdef __UART_DEBUG
//						writeStringToUxRTx( UART_DEBUG, "\r\nConnected to server\r\n" );
//#endif
//						*gsmStatus |= GSM_CONNECTED_TO_SERVER;
//						break;
//					}
//					else
//					{
//						*gsmStatus &= ~GSM_CONNECTED_TO_SERVER;
//						downloadFromServer++;
//						if( downloadFromServer > 2 )
//							downloadFromServer = 0;
//					}
//				}
//				if( retries > 5 )
//					totalRetries++;
//			}
//			else
//			{
//				totalRetries++;
//#ifdef __UART_DEBUG
//				writeStringToUxRTx( UART_DEBUG, "\r\nGPRS not established\r\n" );
//#endif
//			}
//
//			/* Download program file  */
//			/* If connected to server */
//			if( *gsmStatus & (GSM_CONNECTED_TO_SERVER) )
//			{
//				/* Start downloading to address 0x8 in external FLASH memory */
//				//if( (gsmDownloadFileHttpGet( 0x1, downloadFromServer, IMEI )) ) /* We have several servers to try to download from */
//				switch( downloadFromServer )
//				{
//					case 0:	if( gsm_download_file_http_get( 0x01, "/", IMEI, "1.0", "atcindu.carlssonia.org" ) )
//								*gsmStatus |= GSM_GOT_HTTP;
//							else
//								*gsmStatus &= ~GSM_GOT_HTTP;
//							break;
//					case 1:	if( gsm_download_file_http_get( 0x01, "/~hborg/", IMEI, "1.1", "www.isk.kth.se" ) )
//								*gsmStatus |= GSM_GOT_HTTP;
//							else
//								*gsmStatus &= ~GSM_GOT_HTTP;
//							break;
//					case 2:	if( gsm_download_file_http_get( 0x01, "/henrikborg/", IMEI, "1.1", "hem.bredband.net" ) )
//								*gsmStatus |= GSM_GOT_HTTP;
//							else
//								*gsmStatus &= ~GSM_GOT_HTTP;
//							break;
//				}
//				if( *gsmStatus &= GSM_GOT_HTTP )
//				{
////					*gsmStatus |= GSM_GOT_HTTP;
////					#ifdef	__DEBUG
//#ifdef __UART_DEBUG
//					writeStringToUxRTx( UART_DEBUG, "\r\nFile downloaded\r\n" );
//#endif
////					#endif
//					DelayMs(1000);
//					/* Reboot */
//					SoftReset();
//					/* For safty reasons                               */
//					/* So there will be only nop() in execution buffer */
//					while(1);
//		//			goto fileDownloaded;
//				}
//				else
//				{
////					*gsmStatus &= ~GSM_GOT_HTTP;
////					#ifdef	__DEBUG
//#ifdef __UART_DEBUG
//					writeStringToUxRTx( UART_DEBUG, "\r\nFile not found\r\n" );
//#endif
////					#endif
//					downloadFromServer++;
//					totalRetries++;
//				}
//			}
//			else
//				totalRetries++;
//
////			if( *gsmStatus & GSM_GOT_HTTP )
////				/* Should be a reboot */
////				goto newFirmwareDownloaded;
//
//			/* Now we should not be connected to any server */
//			*gsmStatus &= ~GSM_CONNECTED_TO_SERVER;
//
//			/* Establish GPRS - including initialization of GPRS */
//			/* If connected to mobile network                    */
//			if( gsmConnectedToMobileNetwork( gsmStatus, AtCind ) )
//			{
//				for( i = 0; i < 2; i = i + 1 )
//					if( gsmEstablishGPRS() )
//						break;
//
//				if( i < 2 )
//					*gsmStatus |= GSM_GPRS_ESTABLISHED;
//				else
//				{
//					*gsmStatus &= ~GSM_GPRS_ESTABLISHED;
//					totalRetries++;
//				}
//			}
//			else
//				totalRetries++;
//		}
//		/* Initialize GSM module               */
//		/* Switch of GSM module and restart it */
//		else /* totalRetries > 10 */
//		{
//			/* Turn off GSM module                                                             */
//			/* Try 5 times soft turn off - will disconect from mobile operator in a proper way */
//			/* Try 5 times hard turn off - emergency only                                      */
//			//if( gsmSoftTurnOff( 5 ) )
//			gsmSoftTurnOff( 5 );
//				gsmHardTurnOff( 5 );
//
//			/* Restart GSM module */
//			for( i = 0; i < 4; i++ )
//				if( gsmInit( gsmStatus, IMEI, IMSI, AtCind  ) )	/* Get International Mobile Equipment Identity from GSM module */
//					break;
//
//			/* Reset variable and start a fresh set of download tryouts */
//			totalRetries = 0;
//			downloadFromServer = 0;
//		}
//	}
//}

/********************************************************************************************************************
* On the end of each row the name of the belonging functionname is named
* int gsm_connect_and_download_firmware(	unsigned int* gsmStatus, char* AtCind,
* 										char* serverToCallString, char* port, int maxConnectionTries,
* 										unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString
* 									)
*
* Arguments:    Used in calling gsmConnectedToMobileNetwork()
*               gsmStatus          - Address to gsmStatus variable (pointer)
*               AtCind             - Pointer to AtCind vector (pointer)
*
*               Used in calling gsmConnectToServer()
*               serverToCallString - The name of the server
*               port               - The IP port on server
*               maxConnectionTries - The number of times the module will try yo establish a connection to remote host
*
*               Used in calling gsmDownloadFileHttpGet()
*               address            - The address in external FLASH memory, SST25, to store the ICP-file
*               fileDirectory      - The directory on server there the file is stored, starts allways with "/"
*               fileName           - The name of the file. In this case the IMEI number
*               httpVersion        - The version of the supported HTTP protocol. List of currently supplyed versions [1.0;1,1]
*               hostString         - The name of the server there file is stored. Might be different then serverToCallString
*
* Function is non-locking
********************************************************************************************************************/
//int gsm_connect_and_download_firmware(	unsigned int* gsmStatus, char* AtCind,															/* gsmConnectedToMobileNetwork() */
//									char* serverToCallString, char* port, int maxConnectionTries,											/* gsmConnectToServer()          */
//									unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString	/* gsmDownloadFileHttpGet()      */
//								 )
//{
int gsm_connect_and_download_firmware(
    struct connection_state_t* conn_state,
    char* AtCind,
    int maxConnectionTries) {

	int nError = 0;

	struct http_request_t* http_request = &(conn_state->request);
	unsigned int* gsmStatus = &(conn_state->gsmStatus);

	if (gsmConnectedToMobileNetwork(gsmStatus, AtCind)) {
		if (gsmEstablishGPRS()) {
			*gsmStatus |= GSM_GPRS_ESTABLISHED;
			/* int gsmConnectToServer( char* server, char* port, int maxTries ); */
			if (gsmConnectToServer( http_request->server, http_request->port, maxConnectionTries, 0)) {
				*gsmStatus |= GSM_CONNECTED_TO_SERVER;
				if (gsm_download_file_http_get(conn_state)) {
					*gsmStatus |= GSM_GOT_HTTP;
					nError = 1;
				} else {
					*gsmStatus &= ~GSM_GOT_HTTP;
				}
			} else {
				*gsmStatus &= ~GSM_CONNECTED_TO_SERVER;
			}
		} else {
			*gsmStatus &= ~GSM_GPRS_ESTABLISHED;
		}
	}

	return nError;
}

int gsm_connect_and_download_extended_par(
    struct connection_state_t* conn_state,
    char* AtCind, int maxConnectionTries,
    unsigned int address) {

  int nError = 0;
  struct http_request_t* http_request = &(conn_state->request);

  unsigned int* gsmStatus = &(conn_state->gsmStatus);

  if (gsmConnectedToMobileNetwork(gsmStatus, AtCind)) {
    if (gsmEstablishGPRS()) {
      *gsmStatus |= GSM_GPRS_ESTABLISHED;
      if (gsmConnectToServer(http_request->server, http_request->port,
          maxConnectionTries, 0)) {
        *gsmStatus |= GSM_CONNECTED_TO_SERVER;
        if (gsm_download_extended_par(conn_state)) {
          *gsmStatus |= GSM_GOT_HTTP;
          nError = 1;
        } else {
          *gsmStatus &= ~GSM_GOT_HTTP;
        }
      } else {
        *gsmStatus &= ~GSM_CONNECTED_TO_SERVER;
      }
    } else {
      *gsmStatus &= ~GSM_GPRS_ESTABLISHED;
    }
  }

  return nError;
}

//TODO DY Legacy code - to be removed after review
int gsmDownloadFileHttpGet_Par( unsigned int address, struct connection_state_t* conn_state, struct http_request_t* httpRequest)
{
	char* fileDirectory = httpRequest->fileDirectory;
	char* fileName = httpRequest->fileName;
	char* httpVersion = httpRequest->httpVersion;
    char* hostString = httpRequest->server;

    BYTE* local_par_file_string = conn_state->data_store.par_file_string;
    BYTE* local_par_file_mask = conn_state->data_store.par_file_mask;

	/* int address - Start write address for download*/

	/* Local defines */
//	#define semBufferMax	1024
//	#define semBufferMax	1023
//	int semBuffer					= semBufferMax;	/* Number of unused bytes in buffer[] */
//	char semBurningInProgress		= 0;	/* '1' if burning in external FLASH memory is in porgress */
//	char semReachedChecksumInFile	= 0;
	int i							= 0;	/* Reserved counter variable */
	int j							= 0;	/* Reserved counter variable */
	int n							= 0;	/* Reserved counter variable */
//	char buffer[semBuffer];				/* Received characters not yet pushed to external FLASH memory */
//	char buffer[semBuffer + 1];				/* Received characters not yet pushed to external FLASH memory */
	char dropped					__attribute__ ((aligned (8)))	= 0;	/* '1' if received character could not be buffered */
//	char c							__attribute__ ((aligned (8)));			/* Temporary character variable */ //REMOVED
	BYTE c							__attribute__ ((aligned (8)));			/* Temporary character variable */ //ADDED
//	char end						= 0;
//	char nError						= 1;	/* Indicates if no errors have occured */
//	char sectorEraseBegun			= 0;	/* Indicates whether An erase of sector have begun */
//	unsigned int dataLength			= 0;	/* Number of byte written to external FLASH memory */
//	unsigned int calculatedChecksum	= 0;
//	unsigned int downloadedChecksum	= 0;
	unsigned int gsmStatusFake		= 0;
	char	AtCindFake[22]			__attribute__ ((aligned (32)));
	char	IMEIFake[16]			__attribute__ ((aligned (8)));
	char	IMSIFake[16]			__attribute__ ((aligned (8)));
//	unsigned int orgAddress			= address;
	UINT32	length					= 0;
//	UINT32	start_position			= 0;
//	int		sem_position			= 2;
	int		sem_high_nibble			= 1;
	BYTE	tmp_byte				__attribute__ ((aligned (8)));
	int		sem_string_mask			= 1;
	//BYTE	heat_buffer[ _global_heat_buffer_length ]	__attribute__ ((aligned (8)));
	int		characterReceived;

	//memset( heat_buffer, 0, _global_heat_buffer_length );

	clrUARTRxBuffer( UART_DEBUG );

	//DY
	TRACE_CALL_ENTER();

#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx( UART_DEBUG, "\r\naddress: " );
	puthex32( address );
	writeStringToUxRTx( UART_DEBUG, "\r\nfileDirectory: " );
	writeStringToUxRTx( UART_DEBUG, fileDirectory );
	writeStringToUxRTx( UART_DEBUG, "\r\nfileName: " );
	writeStringToUxRTx( UART_DEBUG, fileName );
	writeStringToUxRTx( UART_DEBUG, "\r\nhttpVersion: " );
	writeStringToUxRTx( UART_DEBUG, httpVersion );
	writeStringToUxRTx( UART_DEBUG, "\r\nhostString: " );
	writeStringToUxRTx( UART_DEBUG, hostString );
	writeStringToUxRTx( UART_DEBUG, "\r\n" );
#endif

	/* Prepare for first write */
	SST25CSHigh();
	//SST25SectorErase( SST25_CMD_SER04, address );
	//SST25SectorErase( SST25_CMD_SER04, 0x0 );
	//SST25PrepareWriteAtAddress( address );
	//SST25PrepareWriteAtAddress( 0x0 );

	/* Make sure the first sector has been erased */
	/* In case address is not even sector addres  */
	SST25SectorErase( SST25_CMD_SER04, address & ~sectorSize );
	SST25PrepareWriteAtAddress( address );

#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx( UART_DEBUG, "\r\nStart downloading file" );
	writeStringToUxRTx( UART_DEBUG, "\r\nSending HTTP/GET\r\n" );
#endif
	LED_SERVICE = 0;
	clrUARTRxBuffer( UART_GSM );

#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx( UART_DEBUG, "GET " );
	writeStringToUxRTx( UART_DEBUG, fileDirectory );
	writeStringToUxRTx( UART_DEBUG, fileName );
	writeStringToUxRTx( UART_DEBUG, " HTTP/" );
	writeStringToUxRTx( UART_DEBUG, httpVersion );
	writeStringToUxRTx( UART_DEBUG, " \r\nHost: " );
	writeStringToUxRTx( UART_DEBUG, hostString );
	writeStringToUxRTx( UART_DEBUG, "\r\n\r\n" );
#endif
	writeStringToUxRTx( UART_GSM, "GET " );
	writeStringToUxRTx( UART_GSM, fileDirectory );
	writeStringToUxRTx( UART_GSM, fileName );
	writeStringToUxRTx( UART_GSM, " HTTP/" );
	writeStringToUxRTx( UART_GSM, httpVersion );
	writeStringToUxRTx( UART_GSM, " \r\nHost: " );
	writeStringToUxRTx( UART_GSM, hostString );
	writeStringToUxRTx( UART_GSM, "\r\n\r\n" );

	clrUARTRxBuffer( UART_GSM );

	/* Check if file exist */
	/* Parse HTTP header  */
	if( waitForString( "HTTP/1.", delay50s ) )
		if( recUART( UART_GSM, &c ) )
			if( waitForString( " 200", delay10ms ) )
				goto receiveFile;

	/* If file does not exist */
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
	waitForString( "3\r", delay30s );
#else
	waitForString( "NO CARRIER", delay30s );
#endif
	{
		LED_SERVICE_ON;

		TRACE_CALL_EXIT_PARAM_INT(0);
		return 0;
	}

	receiveFile:
	/* Wait for content/file body */
	for( i = 0, j = 0, n = 0; (i < 1048) && (n < 2); i++)
	{
		characterReceived = 0;
		WriteCoreTimer(0);
		// Wait for character
		do
		{
			// HB
			/* Changed to DEBUG port for test of behaivour then getting ERROR string */
			if(comm_uart_is_received_data_available(UART_GSM))
			{
				characterReceived = 1;
				break;
			}
		}
		while(ReadCoreTimer() < delay10s);
		if( !characterReceived )
			break;	// No character in UART_Rx register in time.

		recUART(UART_GSM, &c);

#ifdef __UART_DEBUG
		// Echo to UART_DEBUG
		writeByteToUxRTx(UART_DEBUG, c);
#endif

		// Search for [\r\n\r\n] or [\n\n].
		if( c == '\n' )	// Did we receive a '\n'?
		{
			n++;
			j = 0;
		}
		else			// No we did not.
		{
			if( (c == '\r') && !j )
			{
				j++;
			}
			else
			{
				j = 0;
				n = 0;
			}
		}
	}
	i = 0;
	j = 0;
#ifdef __UART_DEBUG_DOWNLOAD
	writeByteToUxRTx(UART_DEBUG, '#');	// Just a marker in the ouput.
#endif
	if( n < 2 ){
		TRACE_CALL_EXIT_PARAM_INT(0);
		return 0;
	}

//	c = 0;	//qqq
	WriteCoreTimer( 0 );
	while( 1 )
	{
		/* Do not wait too long for a new character */
		if( ReadCoreTimer() > delay50s )
		{
#ifdef __UART_DEBUG
			writeStringToUxRTx( UART_DEBUG, "\r\nTIMEOUT\r\n" );
#endif

			/* Turn off GSM module      */
			/* The hard way is required */
			gsmHardTurnOff( 5 );

			/* Restart GSM module */
			for( i = 0; i < 4; i++ )
				if( gsm_init( &gsmStatusFake, IMEIFake, IMSIFake, AtCindFake  ) )	/* Get International Mobile Equipment Identity from GSM module */
					break;
			//return 0;
			goto dropped;
		}

		if( comm_uart_is_received_data_available( UART_GSM ) )
		{
			/* Reset timer */
			WriteCoreTimer( 0 );

			recUART( UART_GSM, &c );
#ifdef __UART_DEBUG_DOWNLOAD
			writeByteToUxRTx( UART_DEBUG, c );
			if( c == '\n' )
				writeByteToUxRTx( UART_DEBUG, '\r' );
#endif
			//if( (c == END_OF_FILE) || (c == '\r') || (c == '\n') || (length >= _global_heat_buffer_length) )
			if( (c == END_OF_FILE || (!sem_string_mask && length == RECORD_LENGTH)) )
			{
				LED_SERVICE = 0;
#ifdef __UART_DEBUG_DOWNLOAD
				if( c == END_OF_FILE)
					writeStringToUxRTx( UART_DEBUG, "\r\n.par-file downloaded\r\n" );
#endif

//				/* Copy to global record */
//				for( i = 32 + start_position; (i < 32 + length) && (i < RECORD_LENGTH); i++ )
//					_record[i] = heat_buffer[i-32];
				_sem_par_file_end = length;
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                                waitForString( "3\r", delay30s );
#else
				waitForString( "NO CARRIER", delay30s );
#endif
				TRACE_CALL_EXIT_PARAM_INT(1);
				return 1;
			}

			if( isxdigit(c) )
			{
				tmp_byte <<= 4;
				asciihex2binChar( &c );
				tmp_byte += c;

				if( sem_high_nibble )
				{
					sem_high_nibble = 0;
				}
				else
				{
					sem_high_nibble = 1;
					//_global_heat_buffer[length++] = tmp_byte;
					if( sem_string_mask )
					{
						//_global_heat_buffer[length++] = tmp_byte;
						local_par_file_string[length++] = tmp_byte;
						if( (length == RECORD_LENGTH) )
						{
							sem_string_mask = 0;
							length = 0;
						}
					}
					else
					{
						local_par_file_mask[length++] = tmp_byte;
					}
					//heat_buffer[length++] = tmp_byte;
				}
			}

		}
	}

	dropped:
	if( dropped )
	{
#ifdef __UART_DEBUG_DOWNLOAD
		writeStringToUxRTx( UART_DEBUG, "\r\n!!! DROPPED !!!" );
#endif
		WriteCoreTimer( 0 );
		// Wait for EndOfFile-character
		do
		{
			if (comm_uart_is_received_data_available( UART_GSM ) )
	    	{
		    	if( recUART( UART_GSM, &c ) )
		    		if( c == '.' )
	    				goto dropped_after;

		    	WriteCoreTimer( 0 );
	    	}
		}while( ReadCoreTimer() < delay3s );

		dropped_after:
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "3\r", delay30s );
#else
		waitForString( "NO CARRIER", delay30s );
#endif

		LED_SERVICE = 0;

		TRACE_CALL_EXIT_PARAM_INT(0);
		return 0;
	}

//	LED_SERVICE = 0;
	LED_SERVICE_ON;

	TRACE_CALL_EXIT_PARAM_INT(0);
	return 0;
}

int add_and_erase_sector(connection_state_t* conn_state, uint32_t address) {
  int i = 0;
  int new_sector = address / sectorSize;
  for (i = 0; i < conn_state->sectors.sectors_size; i++) {
    if (conn_state->sectors.sectors[i] == new_sector) {
      //This sector has already been erased in this transaction.
      LOGD("Sector %d for address %ux has already been erased!", new_sector,
          (unsigned int)address);
      break;
    }
  }

  if (i == conn_state->sectors.sectors_size) {
    //TODO sector buf size control.
    conn_state->sectors.sectors[conn_state->sectors.sectors_size++] =
        new_sector;
    LOGD("Erasing the sector %d for address %ux", new_sector, (unsigned int)address);
    SST25SectorErase(SST25_CMD_SER04, address);

    return 1;
  }

  return 0;
}

void http_parse_extended_par_line(connection_state_t* conn_state, const uint8_t* line) {
  int write_char_by_char = 0;
  uint32_t address = 0;

  uint8_t temp[100];
//    uint16_t temp_hex[10];

  LOGD("###############");
  LOGD("line:%s", line);

  uint8_t* end = multi_strchr(line, (uint8_t*)"/\r\n");
  if (end) {
    *end = 0;
  } else {
    return;
  }

  line = skip_whitespace(line);
  LOGD("line2:%s", line);

  if (!line || !(*line)) {
    return;
  }

  //  num_of_items_read = sscanf_s(line, "@%x ", &address);

  if (line[0] == '@') {
    write_char_by_char = 1;
    line++;
  }

  line = read_hex(line, &address);
  if (!line) {
    LOGD("No address!");
    return;
  }

  LOGD("address %ux directwrite %d", (unsigned int)address, write_char_by_char);

  add_and_erase_sector(conn_state, address);

  if (!write_char_by_char) { //hex data
    int total = 0;
    int i = 0;
    for (; line && *line; line++) {
//          line = read_hex(line, &li1);
//          if (!line){
//              LOGD("Done!!!");
//              break;
//          } else {
//              LOGD("Data Hex: %lx", li1);
//              temp_hex[i++] = (uint16_t)li1;
//          }
      if (!isxdigit(*line)) {
        continue;
      }

      temp[total++] = *line;
    }
    temp[total] = 0; //safety

    int j = 0;
    for (i = 0; i < total - 1; i += 2, j++) {
//            LOGD("first %c second %c", temp[i], temp[i + 1]);
      temp[j] = asciihexbytes_to_byte(temp[i], temp[i + 1]);

//            LOGD("Data Hex (j: %d) %x", j, (unsigned int)temp[j]);
    }
    temp[j] = 0; //safety

    LOGD("Write following data to %ux", (unsigned int)address);
//        print_buffer_hex(temp, j);
    writeStringToSST25(address, temp, j);
  } else {
    int i = 0;
    for (; line && *line; line++) {
      if (isspace(*line)) {
        continue;
      }
      temp[i++] = *line;
//            LOGD("Data Byte:%c", (int)*line);
    }

//        LOGD("Write following data to %lx", address);
    writeStringToSST25(address, temp, i);
  }

  return;
}

int http_parse_par_file_legacy(connection_state_t* conn_state) {
  uint8_t tmp_byte __attribute__ ((aligned (4)));
  int sem_high_nibble = 1;
  int sem_string_mask = 1;
  uint8_t* par_file_string = conn_state->data_store.par_file_string;
  uint8_t* par_file_mask = conn_state->data_store.par_file_mask;
  uint32_t length = 0;

  int parse_ok = 0;

  //TODO memory mapping!
  long int address = RECORDVARIABLES_START;

  uint8_t* p = conn_state->response.payload;
  uint8_t c;

  TRACE_CALL_ENTER()
  ;

  int i = 0;
  for (i = 0; p + i; i++) {
    c = p[i];

    if ((c == END_OF_FILE || (!sem_string_mask && length == RECORD_LENGTH))) {
      _sem_par_file_end = length;

      //Extended part is right after legacy part
      conn_state->response.extended = &p[i + 1];

      parse_ok = 1;
      break;
    }

    if (isxdigit(c)) {
      tmp_byte <<= 4;
      asciihex2binChar((BYTE*)&c);
      tmp_byte += c;

      if (sem_high_nibble) {
        sem_high_nibble = 0;
      } else {
        sem_high_nibble = 1;
        //_global_heat_buffer[length++] = tmp_byte;
        if (sem_string_mask) {
          //_global_heat_buffer[length++] = tmp_byte;
          par_file_string[length++] = tmp_byte;
          if ((length == RECORD_LENGTH)) {
            sem_string_mask = 0;
            length = 0;
          }
        } else {
          par_file_mask[length++] = tmp_byte;
        }
      }
    }
  }

  if (parse_ok) {
    //TODO check if the par file and mask has the correct size
    add_and_erase_sector(conn_state, address);
    writeStringToSST25(address, par_file_string, length);
    writeStringToSST25(address + length, par_file_mask, length);

    TRACE_CALL_EXIT_PARAM_INT(i);
    return 1;
  }

  TRACE_CALL_EXIT_PARAM_INT(0);
  return 0;
}

int http_parse_par_file_extended(connection_state_t* conn_state) {
  TRACE_CALL_ENTER();
  LOGD("Extended: %s", conn_state->response.extended);

  uint8_t* data = conn_state->response.extended;

  TRACE_CALL_ENTER();
  LOGD("Extended: %s", data);

  uint8_t* line = data;
  uint8_t* next_line = NULL;

  while (line) {
    char c = *line;

    if (isspace(c)) {
      line++;
      continue;
    }

    next_line = multi_strchr(line, (uint8_t*)"\r\n");
    if (next_line) {
      *(next_line++) = 0;
    }
    http_parse_extended_par_line(conn_state, line);
    line = next_line;
  }

  return 1;
}

//Verify CRC of payload and then parse the par file in the payload
int http_parse_par_file(connection_state_t* conn_state) {

  uint8_t* payload = conn_state->response.payload;
  uint8_t* par_file_end = NULL;

  par_file_end = (uint8_t*)strrchr((const char*)payload, '$');

  if (par_file_end) {
    uint32_t crc_incoming = 0;
    uint8_t* crc_start = ++par_file_end;
    LOGD("Incoming crc str: %s", crc_start);

    if (read_hex(crc_start, &crc_incoming)) {
      LOGD("Incoming crc long: %x", (unsigned int)crc_incoming);

      //Now that we read CRC, we can remove it from payload.
      *crc_start = 0;
      conn_state->response.payload_len = crc_start - payload;

      LOGD("payload len: %d", conn_state->response.payload_len);

      int i = 0;
      for (i = 0; i < conn_state->response.payload_len ; i++) {
        char c = conn_state->response.payload[i];
//        LOGD("Char: %c %x %d", c, c, c);
      }

      uint32_t crc_calculated = 0;

      if (!crc_hw_32(conn_state->response.payload,
          conn_state->response.payload_len, &crc_calculated)) {
        LOGD("CRC Calculation (HW) Fail!");
        return 0;
      }

      LOGD("crc_calculated (%c) %x",
          conn_state->response.payload[conn_state->response.payload_len - 1],
          (unsigned int)crc_calculated);
      if (crc_incoming == crc_calculated) {
        LOGD("Verified Payload (%d): %s", conn_state->response.payload_len,
            conn_state->response.payload);

        //parse the extended part only if the legacy parsing is successful.
        if (http_parse_par_file_legacy(conn_state)) {
          return http_parse_par_file_extended(conn_state);
        }
      }
    }
  }

  LOGD("Invalid Par file!");
  return 0;
}

int http_handle_response(connection_state_t* conn_state) {
  uint8_t* inputbuf = NULL;
  uint8_t* colon = NULL;
  uint8_t* end_of_line = NULL;
  int buf_size = 0;

  inputbuf = conn_state->inputbuf;
  buf_size = conn_state->inputbuf_size;

  if (strncmp((const char*)inputbuf, "HTTP/1.1 200", strlen("HTTP/1.1 200")) != 0) {
    LOGD("Not HTTP");
    return 0;
  }

  do {
    colon = (uint8_t*)strchr((const char*)inputbuf, ':');
    if (colon) {
      end_of_line = (uint8_t*)strchr((const char*)colon, '\n');
      inputbuf = end_of_line;
    } else {
//      inputbuf++;
//      if (inputbuf && *inputbuf == '\n') {
//        inputbuf++;
//      }
      inputbuf = skip_blank_lines(inputbuf);
      break;
    }
  } while (inputbuf);

  if (inputbuf) {
    conn_state->response.payload = inputbuf;
    LOGD("Payload: %s", conn_state->response.payload);

    return http_parse_par_file(conn_state);
  }

  TRACE_CALL_EXIT_PARAM_INT(0);
  return 0;
}

//TODO handling gsm error text??
int gsm_get_http_response(connection_state_t* conn_state) {
  int result = 0;
//    int par_file_size = 0;
  int end_of_crc = 0;
  BYTE c __attribute__ ((aligned (4)));

  TRACE_CALL_ENTER()
  ;

  WriteCoreTimer(0);

  //Loop until data is available or timeout in 10 secs
  while (!comm_uart_is_received_data_available(UART_GSM)
      && ReadCoreTimer() < delay10s)
    ;

  if (ReadCoreTimer() >= delay10s) {
    result = 1;
    TRACE_CALL_EXIT_PARAM_INT(1);
    return result;
  }

  WriteCoreTimer(0);
  //Wait 5 seconds until all data is available.
  do {
    while (comm_uart_is_received_data_available(UART_GSM)) {
      if (recUART(UART_GSM, &c)) {
        //TODO buffer size check!
        //TODO add check for the content size
        conn_state->inputbuf[conn_state->inputbuf_size++] = c;

        //TODO is this safe enough?
        /*if (c == '$'){
         par_file_size = conn_state->inputbuf_size;
         } else*/if (c == '^') {
          end_of_crc = 1;
        }
      }
    }
  } while (ReadCoreTimer() < delay5s && !end_of_crc);

  conn_state->inputbuf[conn_state->inputbuf_size] = 0; //safety measure
  //TODO wait for NO CARRIER? Error case?
  //gsm_wait_for_response(GSM_RESPONSE_CODE_NO_CARRIER, delay30s);

  LOGD("Incoming Data (Size: %d): %s", conn_state->inputbuf_size,
      conn_state->inputbuf);

  if (end_of_crc) {
    //return the par file size
    result = conn_state->inputbuf_size;
  }

  TRACE_CALL_EXIT_PARAM_INT(result);
  return result;
}

void gsm_send_http_request(struct connection_state_t* conn_state) {
  struct http_request_t* http_request = &(conn_state->request);
  char* fileDirectory = http_request->fileDirectory;
  char* fileName = http_request->fileName;
  char* httpVersion = http_request->httpVersion;
  char* hostString = http_request->server;

  //TODO size check.
  sprintf((char *)conn_state->outputbuf, "GET %s%s HTTP/%s \r\nHost: %s\r\n\r\n",
      fileDirectory, fileName, httpVersion, hostString);
  LOGD("HTTP Request: %s", conn_state->outputbuf);

  writeStringToUxRTx(UART_GSM, (char*)conn_state->outputbuf);
  clrUARTRxBuffer(UART_GSM);
}

int gsm_handle_http_response(struct connection_state_t* conn_state) {
  if (gsm_get_http_response(conn_state) == 0) {
    LOGD("No or invalid response!");
    return 0;
  }

  return http_handle_response(conn_state);
}

//TODO timer stuff? Do not wait too much for the data.
int gsm_download_extended_par(struct connection_state_t* conn_state) {
  /* int address - Start write address for download*/
  TRACE_CALL_ENTER();

  gsm_send_http_request(conn_state);

  return gsm_handle_http_response(conn_state);
}

int gsmDownloadTest_HttpGet(	unsigned int* gsmStatus, char* AtCind,															/* gsmConnectedToMobileNetwork() */
									char* serverToCallString, char* port, int maxConnectionTries,											/* gsmConnectToServer()          */
									unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString	/* gsm_download_file_http_get()      */
								 )
{
	int nError = 0;

	if( gsmConnectedToMobileNetwork( gsmStatus, AtCind ) )
	{
		if( gsmEstablishGPRS() )
		{
			*gsmStatus |= GSM_GPRS_ESTABLISHED;
			/* int gsmConnectToServer( char* server, char* port, int maxTries ); */
			if( gsmConnectToServer( serverToCallString, port, maxConnectionTries, 0 ) )
			{
				*gsmStatus |= GSM_CONNECTED_TO_SERVER;
				if( gsmDownloadTestHttpGet( address,  fileDirectory, fileName, httpVersion, hostString ) )
				{
					*gsmStatus |= GSM_GOT_HTTP;
					nError = 1;
			}
				else
					*gsmStatus &= ~GSM_GOT_HTTP;
			}
			else
				*gsmStatus &= ~GSM_CONNECTED_TO_SERVER;
		}
		else
			*gsmStatus &= ~GSM_GPRS_ESTABLISHED;
	}

	return nError;
}

int gsmDownloadTestHttpGet( unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString )
{
	/* int address - Start write address for download*/

	/* Local defines */
//	#define semBufferMax	1024
//	#define semBufferMax	1023
//	int		semBuffer					= semBufferMax;	/* Number of unused bytes in buffer[] */
//	char	semBurningInProgress		__attribute__ ((aligned (8)))		= 0;	/* '1' if burning in external FLASH memory is in porgress */
	char	semReachedChecksumInFile	__attribute__ ((aligned (8)))	= 0;
//	int		i							= 0;	/* Reserved counter variable */
//	int		j							= 0;	/* Reserved counter variable */
//	char buffer[semBuffer];				/* Received characters not yet pushed to external FLASH memory */
//	char	buffer[semBuffer + 1]		__attribute__ ((aligned (8)));				/* Received characters not yet pushed to external FLASH memory */
	char	dropped						__attribute__ ((aligned (8)))	= 0;	/* '1' if received character could not be buffered */
//	char	c							__attribute__ ((aligned (8)));									/* Temporary character variable */ //REMOVED
	BYTE	c							__attribute__ ((aligned (8)));									/* Temporary character variable */ //ADDED
	char	end							__attribute__ ((aligned (8)))	= 0;
//	char nError						= 1;	/* Indicates if no errors have occured */
//	char	sectorEraseBegun			__attribute__ ((aligned (8)))	= 0;	/* Indicates whether An erase of sector have begun */
	unsigned int dataLength			= 0;	/* Number of byte written to external FLASH memory */
//        unsigned int dataLength3		= 0;
        unsigned int dataLengthTotal		= 0;
        unsigned int numberOfRtsAsserted        = 0;
//	unsigned int calculatedChecksum	= 0;
//	unsigned int downloadedChecksum	= 0;
//	unsigned int gsmStatusFake		= 0;
//	char AtCindFake[22]					__attribute__ ((aligned (32)));
//	char IMEIFake[16]					__attribute__ ((aligned (8)));
//	char IMSIFake[16]					__attribute__ ((aligned (8)));
//	unsigned int orgAddress		= address;
#ifdef __UART_DEBUG
	unsigned int dataLengthInRTS		= 0;
#endif

	clrUARTRxBuffer( UART_DEBUG );

#ifdef __UART_DEBUG_DOWNLOAD
	writeStringToUxRTx( UART_DEBUG, "\r\nStart downloading file" );
	writeStringToUxRTx( UART_DEBUG, "\r\nSending HTTP/GET\r\n" );
#endif
	clrUARTRxBuffer( UART_GSM );
	writeStringToUxRTx( UART_GSM, "GET " );
	writeStringToUxRTx( UART_GSM, fileDirectory );
	writeStringToUxRTx( UART_GSM, fileName );
	writeStringToUxRTx( UART_GSM, " HTTP/" );
	writeStringToUxRTx( UART_GSM, httpVersion );
	writeStringToUxRTx( UART_GSM, " \r\nHost: " );
	writeStringToUxRTx( UART_GSM, hostString );
	writeStringToUxRTx( UART_GSM, "\r\n\r\n" );

	clrUARTRxBuffer( UART_GSM );

        /* Check if file exist */
	/* Parse HTTP headder  */
	if( waitForString( "HTTP/1.", delay50s ) )
		if( recUART( UART_GSM, &c ) )
		{
#ifdef __UART_DEBUG
			writeByteToUxRTx( UART_DEBUG, c );
#endif
			if( waitForString( " 200", delay10ms ) )
				goto receiveFile;
		}

	/* If file does not exist */

#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
        waitForString( "3\r", delay30s );
#else
	waitForString( "NO CARRIER", delay30s );
#endif
	return 0;

	receiveFile:
	/* Wait for content/file body */
	if( !waitForString( "\r\n\r\n", delay3s ) )
		return 0;

        WriteCoreTimer( 0 );
	while( 1 )
	{
//		nextChar:

//		/* Do not wait to long for a new character */
//		if( ReadCoreTimer() > delay50s )
//		{
//			writeStringToUxRTx( UART_DEBUG, "\r\nTIMEOUT\r\n" );
//
//			//return 0;
//			goto dropped;
//		}

		if( comm_uart_is_received_data_available( UART_GSM ) )
		//if( UARTReceivedDataIsAvailable( UART_DEBUG ) )
		{
//			/* Reset timer */
//			WriteCoreTimer( 0 );

                        c = comm_uart_get_data_byte( UART_GSM );
//                        writeByteToUxRTx( UART_DEBUG, c );

                        dataLength++;
                        dataLengthTotal++;
                        if( dataLength >= 100)
                        {
//                            dataLength3 = 0;
                            nGSM_RTS = 1;
                            LED_SERVICE_INV;
//                            nGSM_CTS = 1;
                            numberOfRtsAsserted++;
                            WriteCoreTimer( 0 );
                            do
                            {
//                                if( UARTReceivedDataIsAvailable( UART_GSM ) )
//                                {
//                                    c = UARTGetDataByte( UART_GSM );
//    //                                dataLength++;
//                                    writeByteToUxRTx( UART_DEBUG, c );
//                                    dataLengthInRTS++;
//                                    dataLength3++;
//                                    dataLengthTotal++;
//                                    if( c == END_OF_FILE )
//                                        goto end_of_file;
//                                }
                            }while( ReadCoreTimer() < delay250ms );
//                            while( UARTReceivedDataIsAvailable( UART_GSM ) )
//                            {
//                                c = UARTGetDataByte( UART_GSM );
////                                dataLength++;
//                                writeByteToUxRTx( UART_DEBUG, c );
//                                dataLengthInRTS++;
//                                dataLength3++;
//                                dataLengthTotal++;
//                                if( c == END_OF_FILE )
//                                    goto end_of_file;
//                            }
//                            writeStringToUxRTx( UART_DEBUG, "\r\n" );
//                            clrUARTRxBuffer( UART_GSM );
//                            DelayMs(1000);
                            dataLength = 0;
                            nGSM_RTS = 0;
//                            nGSM_CTS = 0;
//                            puthex32( dataLength3 );
//                            writeStringToUxRTx( UART_DEBUG, "\r\n" );
                        }

                        /* Check if we have reached to the point in file there the checksum is added */
                        if( c == '$' )
                                semReachedChecksumInFile = 1;

//                        /* Calculate checksum of downloaded file */
//                        if( !semReachedChecksumInFile )
//                        {
//                                calculatedChecksum += (unsigned int)c;
//                        }
//                        else
////      			if( c != '\r' && c != '\n' && c != '$' )
//                        if( c != END_OF_FILE && c != '\r' && c != '\n' && c != CHECKSUM )
//                        {
//                                downloadedChecksum <<= 4;
//                                downloadedChecksum = downloadedChecksum + ((c < 'A') ? c - '0' : ((c < 'a') ? c - 'A' + 0xA : c - 'a' + 0xA ));
//                        }

                        if( c == END_OF_FILE )
                        {
//                            end_of_file:
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                            waitForString( "3\r", delay30s );
#else
                            waitForString( "NO CARRIER", delay30s );
#endif
#ifdef __UART_DEBUG
							writeStringToUxRTx( UART_DEBUG, "\r\n" );
                            puthex32( dataLengthTotal );
                            writeStringToUxRTx( UART_DEBUG, "\r\n" );
                            putdec_debug( dataLengthTotal );
                            writeStringToUxRTx( UART_DEBUG, " bytes downloaded\r\n" );
                            writeStringToUxRTx( UART_DEBUG, "\r\n" );
                            puthex32( dataLengthInRTS );
                            writeStringToUxRTx( UART_DEBUG, " bytes downloaded during RTS\r\n" );
                            writeStringToUxRTx( UART_DEBUG, "\r\n" );
                            puthex32( numberOfRtsAsserted );
                            writeStringToUxRTx( UART_DEBUG, " number of RTS assertion\r\n" );
#endif

                            WriteCoreTimer( 0 );
                            do
                            {
                                if( comm_uart_is_received_data_available( UART_GSM ) )
                                {
                                    c = comm_uart_get_data_byte( UART_GSM );
#ifdef __UART_DEBUG
									writeByteToUxRTx( UART_DEBUG, c );
#endif
                                }
                            }while( ReadCoreTimer() < delay1s );
                            return 1;
                        }

                        /* Test for end of datastream */
                        if( end == 0 && c == END_OF_FILE )
                                end = 8;
                        else if( end == 8 )
                                end = 8;
                        else
                                end = 0;
		}
	}


//	dropped:
	if( dropped )
	{
//		#ifdef __UART_DEBUG_DOWNLOAD
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\n!!! DROPPED !!!" );
#endif
//		#endif
//		i = 0;
//		while(1)
//		{
			WriteCoreTimer( 0 );
			// Wait for EndOfFile-character
			do
			{
				if (comm_uart_is_received_data_available( UART_GSM ) )
		    	{
//			    	i = 0;
			    	if( recUART( UART_GSM, &c ) )
			    		if( c == '.' )
		    				goto dropped_after;

			    	WriteCoreTimer( 0 );
		    	}
			}while( ReadCoreTimer() < delay3s );
			/* More than 10 tries without any received character */
			/* Give up                                           */
//			if( i++ > 9 )
//				break;
//		}
		dropped_after:
//		waitForString( "NO CARRIER", delay5s );
#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
                waitForString( "3\r", delay30s );
#else
		waitForString( "NO CARRIER", delay30s );
#endif
		//nError = 0;
//		dataLength = 0;
		return 0;
		//return dataLength;
	}

//	gsmDownloadFileEnd:
	//return dataLength;
	return 0;
}

////#define maxLengthOfGsmInputBuffer                       1024
////#define maxLengthOfIsmOutputBuffer                      15
//int     _semGsmNumberOfRowsInputBuffer                  = 0;
//int     _semGsmOutputBuffer                             = 0;
//BYTE    _gsmInputBuffer[maxLengthOfGsmInputBuffer + 1]      __attribute__ ((aligned (8)));
//BYTE    _gsmOutputBuffer[maxLengthOfGsmOutputBuffer]    __attribute__ ((aligned (8)));
//int     _semGsmInputBuffer                              = maxLengthOfGsmInputBuffer;
//int     _semGsmPositionPushInputBuffer                  = 0;
//int     _semGsmPositionPullInputBuffer                  = 0;
//int     _semGsmPositionPushOutputBuffer                 = 0;
//int     _semGsmPositionPullOutputBuffer                 = 0;
////char    _scBuffer[maxLengthOfIsmCommandBuffer + 1];
//int     _semGSMCommandReceived                          = 0;
//int     _semGsmPositionPushRadioCommandReceivedBuffer   = 0;
//int     _semGsmPositionPullRadioCommandReceivedBuffer   = 0;
//
//void __ISR( UART_GSM_VECTOR , ipl5) _GsmInputHandler(void)
////void __ISR(_UART_2_VECTOR, ipl5) _IsmInputHandler(void)       //OK
////void __ISR(INT_VECTOR_UART( 5 ), ipl5) _IsmInputHandler(void) //OK
////void __ISR(_UART_3A_VECTOR, ipl5) _IsmInputHandler(void)      // OK
////void __ISR(INT_VECTOR_UART( UART_ISMPIC ), ipl5) _IsmInputHandler(void)
////void __ISR(_UART_5_VECTOR, ipl5) _IsmInputHandler(void)
///****************************************************************************************
//* Interrupt Service Routine
//* Used as low frequency low priority timer
//* UART 5/ISMPIC ISR
//* Specify Interrupt Priority Level = 2, Vector 4
//*
//* UART_INTERRUPT_ON_RX_NOT_EMPTY
//* INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
//* INT_PRIORITY_DISABLED
//* INT_PRIORITY_LEVEL_x -> x=[1;7]
//* INT_SUB_PRIORITY_LEVEL_x -> x=[0;3]
//****************************************************************************************/
//{
//    static int semGSMCommand = 0;
//
//
//	//INT_SOURCE_UART_RX( UART_ISMPIC )
//	//INT_U5RX
//	BYTE indata;
//	// clear the interrupt flag
//	//mT1ClearIntFlag();
//	//UART5
//
////	INTClearFlag( INT_U5RX );
////        INTClearFlag( INT_U2TX );
////        if ( INTGetFlag(INT_U1RX))
////        if ( mU1TXGetIntEnable() &&  mU1TXGetIntFlag() )
//        if( mU1RXGetIntFlag() )
//	{
//            if( UARTReceivedDataIsAvailable ( UART_GSM ) )
//            {
//                    recUART( UART_GSM, (BYTE*)&indata );
//                    INTClearFlag( INT_U1RX );
//
//                    if( indata == '#' )
//                    {
//                        semGSMCommand = 1;
//                    }
//
//                    if( semGSMCommand )
//                    {
//                        if( indata == EOL_GSM )
//                        {
//                            _semGSMCommandReceived++;
//                            semGSMCommand = 0;
////                            _scBuffer[_semGsmPositionPushRadioCommandReceivedBuffer++] = 0;
//                        }
//                        else
//                        {
////                            _scBuffer[_semGsmPositionPushRadioCommandReceivedBuffer++] = indata;
//                        }
//                    }
//                    else
//                    {
//                        if( _semGsmInputBuffer ) /* If there still is room for another character */
//                        {
//                            if( indata == EOL_GSM )
////                            if( isxdigit(indata) )indata == EOL_GSM
//                            {
//                                _gsmInputBuffer[_semGsmPositionPushInputBuffer++] = indata;
//                                _semGsmNumberOfRowsInputBuffer++;
//                                _semGsmInputBuffer--;
//                            }
//                            else
//                            {
//                                _gsmInputBuffer[_semGsmPositionPushInputBuffer++] = indata;
//                                _semGsmInputBuffer--;
//                            }
////                            if( indata == EOL_GSM )
////                          if( indata == '\r' )        // Not ok for GSM.
//    //                        if( indata == EOL_ISMPIC )  // Not tested.
//
//                            _semGsmPositionPushInputBuffer &= maxLengthOfGsmInputBuffer;
//                        }
//                    }
////                    else
////                    {
////                        // For security resons.
////                        // Make sure last written position is '\r'.
////                        // If extracted strlen is to short we now we have a bad record.
////                        if( _semIsmPositionPushInputBuffer )
////                            _ismInputBuffer[_semIsmPositionPushInputBuffer - 1] = '\r';
////                        else
////                            _ismInputBuffer[maxLengthOfIsmInputBuffer] = '\r';
////                    }
//    //		writeByteToUxRTx( UART_DEBUG, indata );
//            }
//        }
//
//        if( mU1TXGetIntFlag() )
//        {
//            for( ; (!(uartReg[UART_GSM]->sta.reg & _U1STA_UTXBF_MASK)) && _semGsmOutputBuffer;  _semGsmPositionPullOutputBuffer++, --_semGsmOutputBuffer )
////            while( !(uartReg[UART_ISMPIC]->sta.reg & _U1STA_UTXBF_MASK) && _ismOutputBuffer )
//                UARTSendDataByte( UART_GSM, _gsmOutputBuffer[_semGsmPositionPullOutputBuffer] );
//
//            // Check if we have put the last character into Tx-buffer.
//            if( !_semGsmOutputBuffer  )
//            {
//                // Restore variables.
//                _semGsmPositionPullOutputBuffer = 0;
////                _semIsmPositionPushOutputBuffer = 0;
//
//                // We have nothing more to send.
//                INTEnable(INT_SOURCE_UART_TX(UART_GSM), INT_DISABLED);
//            }
//
//            INTClearFlag( INT_U1TX );
//        }
//}
//
//int ActOnGsmRows( void )
//{
//    unsigned char   command     = 0;
//    int             returnNr    = 0;
////    char            sc          = 0; //REMOVED
//    BYTE            sc          = 0; //ADDE
//    char            IMEI[16]        __attribute__ ((aligned (4)));
//    char            IMSI[16]        __attribute__ ((aligned (4)));
//
//    gsmGetImei( IMEI );
////    char command[maxLengthOfIsmCommandBuffer + 1];
////    memcpy( command, _scBuffer, strlen(_scBuffer) );
//
////    #define RSSA    "RSSA\r"
////    #define RSUEE   "RSUEE\r"
//
//    if( getCommandNr_2( &command ) == 0 )
//    {
//        /* Remove bad line from buffer */
//        do
//        {
//            if( getCharGsmRowBuffer( &sc ) == 0 )  /* Check if buffer is empty. */
//                return 0;
//        }
//        while( sc != '\n' );
//
//        serveCommand( ERROR, IMEI, IMSI, "Command not understood" );
//    }
//
//    /* Test if command is comming from Server */
////    if( command < 0 )
////        /* Act on info from GSM moduule */
//
//    returnNr = serveCommand( command, IMEI, IMSI, "" );
//
//    return returnNr;
//
////    if( !memcmp( command, "#99", 3) )
////    {
////        memset( _scBuffer, 0, sizeof(_scBuffer) );
////        memset( command, 0, sizeof(command) );
////        memset( _ismOutputBuffer, 0, sizeof(_ismOutputBuffer) );
////        _semIsmOutputBuffer = strlen(RSUEE);
////        memcpy( _ismOutputBuffer, RSUEE, sizeof(RSUEE) );
////        _semRadioCommandReceived = 0;
////        _semIsmPositionPushRadioCommandReceivedBuffer = 0;
////        clrUARTRxBuffer( UART_ISMPIC );
////        INTEnable(INT_SOURCE_UART_TX(UART_ISMPIC), INT_ENABLED);
////
////        // Or:
////        // Untested but should work with a little tweak of shutting IN_Tx down during listening.
//////        ismPicReset();
//////        INTEnable(INT_SOURCE_UART_RX(UART_ISMPIC), INT_DISABLED);
//////        INTEnable(INT_SOURCE_UART_TX(UART_ISMPIC), INT_DISABLED);
//////        writeStringToIsm( "RSSA\r" );
//////        writeStringToIsm( "RSUEE\r" );
//////        INTEnable(INT_SOURCE_UART_RX(UART_ISMPIC), INT_ENABLED);
//////        INTEnable(INT_SOURCE_UART_TX(UART_ISMPIC), INT_ENABLED);
//////        clrUARTRxBuffer( UART_ISMPIC );
////
////        return;
////    }
////    if( !memcmp( command, "#00", 3) )
////    {
////        memset( _scBuffer, 0, sizeof(_scBuffer) );
////        memset( command, 0, sizeof(command) );
////        memset( _ismOutputBuffer, 0, sizeof(_ismOutputBuffer) );
////        _semRadioCommandReceived = 0;
////        _semIsmPositionPushRadioCommandReceivedBuffer = 0;
////        clrUARTRxBuffer( UART_ISMPIC );
////
////        return;
////    }
////    else
////    {
////        _semRadioCommandReceived = 0;
////        INTEnable(INT_SOURCE_UART_TX(UART_ISMPIC), INT_ENABLED);
////    }
//}

/*******************************************************************************
 * Interrupthandler for UART_GSM.
 *
 * Received strings and update some buffers and semaphores.
 ******************************************************************************/
int     _semGsmNumberOfRowsInputBuffer                  = 0;
int     _semGsmOutputBuffer                             = 0;
BYTE    _gsmInputBuffer[maxLengthOfGsmInputBuffer + 1]      __attribute__ ((aligned (8)));
BYTE    _gsmOutputBuffer[maxLengthOfGsmOutputBuffer]    __attribute__ ((aligned (8)));
int     _semGsmInputBuffer                              = maxLengthOfGsmInputBuffer;
int     _semGsmPositionPushInputBuffer                  = 0;
int     _semGsmPositionPullInputBuffer                  = 0;
int     _semGsmPositionPushOutputBuffer                 = 0;
int     _semGsmPositionPullOutputBuffer                 = 0;
char    _scGsmBuffer[maxLengthOfGsmCommandBuffer + 1];
int     _semGsmCommandReceived                        = 0;
int     _semGsmPositionPushGsmCommandReceivedBuffer   = 0;
int     _semGsmATSL                                     = 0;
int     _semGsmATSA                                     = 0;

//void __ISR( UART_GSM_VECTOR , ipl5) _GsmInputHandler(void)
void __ISR(_UART1_VECTOR, ipl5)  _GsmInputHandler(void)
//void __ISR(_UART_2_VECTOR, ipl5) _IsmInputHandler(void)       //OK
//void __ISR(INT_VECTOR_UART( 5 ), ipl5) _IsmInputHandler(void) //OK
//void __ISR(_UART_3A_VECTOR, ipl5) _IsmInputHandler(void)      // OK
//void __ISR(INT_VECTOR_UART( UART_ISMPIC ), ipl5) _IsmInputHandler(void)
//void __ISR(_UART_5_VECTOR, ipl5) _IsmInputHandler(void)
/****************************************************************************************
* Interrupt Service Routine
* Used as low frequency low priority timer
* UART 5/ISMPIC ISR
* Specify Interrupt Priority Level = 2, Vector 4
*
* UART_INTERRUPT_ON_RX_NOT_EMPTY
* INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
* INT_PRIORITY_DISABLED
* INT_PRIORITY_LEVEL_x -> x=[1;7]
* INT_SUB_PRIORITY_LEVEL_x -> x=[0;3]
****************************************************************************************/
{
  static int semGsmCommand = 0;

  //INT_SOURCE_UART_RX( UART_ISMPIC )
  //INT_U5RX
  BYTE indata;
  // clear the interrupt flag
  //mT1ClearIntFlag();
  //UART5
//writeStringToUxRTx( UART_DEBUG, "\r\nInt UART_GSM\r\n" );
//	INTClearFlag( INT_U5RX );
//        INTClearFlag( INT_U2TX );
//        if ( INTGetFlag(INT_U1RX))
//        if ( mU1TXGetIntEnable() &&  mU1TXGetIntFlag() )
  if (mU1RXGetIntFlag()) {
//writeStringToUxRTx( UART_DEBUG, "\r\nInt UART_GSM Receive\r\n" );
    if (comm_uart_is_received_data_available( UART_GSM)) {
//                    recUART( UART_GSM, (BYTE*)&indata );
      indata = comm_uart_get_data_byte( UART_GSM);
//                    INTClearFlag( INT_U2RX );

//                    writeByteToUxRTx( UART_DEBUG, indata );
//                    UARTSendDataByte( UART_DEBUG, indata );

      if (indata == '#') {
        semGsmCommand = 1;
      }

      if (semGsmCommand) {
        if (indata == EOL_GSM) {
          _semGsmCommandReceived++;
          semGsmCommand = 0;
          _scGsmBuffer[_semGsmPositionPushGsmCommandReceivedBuffer++] = 0; // Null at string end.
        } else {
          _scGsmBuffer[_semGsmPositionPushGsmCommandReceivedBuffer++] = indata;
        }
      } else {
        /* If there still is room for another character */
        if (_semGsmInputBuffer) {
//                            if( indata == '\n' )        // Ok.
          // Not tested.
          if (indata == EOL_GSM) {
            _semGsmCommandReceived++;   // TEST
            _gsmInputBuffer[_semGsmPositionPushInputBuffer++] = indata;
            _semGsmNumberOfRowsInputBuffer++;
            _semGsmInputBuffer--;
          } else {
            _gsmInputBuffer[_semGsmPositionPushInputBuffer++] = indata;
            _semGsmInputBuffer--;
          }

          _semGsmPositionPushInputBuffer &= maxLengthOfGsmInputBuffer;
        }
      }
//                    else
//                    {
//                        // For security resons.
//                        // Make sure last written position is '\r'.
//                        // If extracted strlen is to short we now we have a bad record.
//                        if( _semIsmPositionPushInputBuffer )
//                            _ismInputBuffer[_semIsmPositionPushInputBuffer - 1] = '\r';
//                        else
//                            _ismInputBuffer[maxLengthOfIsmInputBuffer] = '\r';
//                    }
      //		writeByteToUxRTx( UART_DEBUG, indata );
    }
    INTClearFlag(INT_U1RX);
  }

  if (mU1TXGetIntFlag()) {
//writeStringToUxRTx( UART_DEBUG, "\r\nInt UART_GSM send\r\n" );
    for (;
        (!(uartReg[UART_GSM]->sta.reg & _U1STA_UTXBF_MASK))
            && _semGsmOutputBuffer;
        _semGsmPositionPullOutputBuffer++, --_semGsmOutputBuffer)
//            while( !(uartReg[UART_GSM]->sta.reg & _U1STA_UTXBF_MASK) && _gsmOutputBuffer )
      comm_uart_send_data_byte( UART_GSM,
          _gsmOutputBuffer[_semGsmPositionPullOutputBuffer]);

    // Check if we have put the last character into Tx-buffer.
    if (!_semGsmOutputBuffer) {
      // Restore variables.
      _semGsmPositionPullOutputBuffer = 0;
//                _semIsmPositionPushOutputBuffer = 0;

      // We have nothing more to send.
      INTEnable(INT_SOURCE_UART_TX(UART_GSM), INT_DISABLED);
    }

    INTClearFlag(INT_U1TX);
  }
}

int gsmExtractReceivedData( char* received_data )
{
    int i;
    int count = 0;
    char string[256];

    // Reset string to all NULLs.
    memset( string, 0, sizeof( string ));

#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\n===== START =====\r\n" );
    // Calculate number of characters in buffer _gsmInputBuffer.
    // Output content of _gsmInputBuffer.
    writeStringToUxRTx( UART_DEBUG, "\r\n_gsmInputBuffer 1:" );//_gsmInputBuffer//_scGsmBuffer
#endif
    DelayMs(1000);
//    memset( string, 0, sizeof( string ));
    // Output the number of bytes in _gsmInputBuffer.

#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\nNumber of EOF_GSM: " );
    putdec_debug( _semGsmCommandReceived );

    writeStringToUxRTx( UART_DEBUG, "\r\ncount: " );
#endif
//        count = 30;
    for( i = 11; isxdigit(_gsmInputBuffer[i]); i++ )
    {
        string[i] = _gsmInputBuffer[i];
#ifdef __UART_DEBUG
		writeByteToUxRTx( UART_DEBUG, _gsmInputBuffer[i] );
        if( _gsmInputBuffer[i] == '\r' )
            writeByteToUxRTx( UART_DEBUG, '\n' );
        if( _gsmInputBuffer[i] == '\n' )
            writeByteToUxRTx( UART_DEBUG, '\r' );
#endif
    }
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, " - " );
    writeStringToUxRTx( UART_DEBUG, &string[11] );
    writeStringToUxRTx( UART_DEBUG, " - " );
#endif
    // convert number to an Int.
//    count = atoi( string + 11 );
    {
        char* ptr = string;
        char** endptr = &ptr;
        count = (int)strtol( string + 11, endptr, 10 );
        if( *endptr == string + 11 ) // Test no digits found.
            count = 0;
    }

#ifdef __UART_DEBUG
//    writeStringToUxRTx( UART_DEBUG, "\r\n" );
    putdec_debug( count );
    writeStringToUxRTx( UART_DEBUG, "\r\n" );

    writeStringToUxRTx( UART_DEBUG, "\r\n36 char's of data in _scGsmBuffer\r\n" );
    for( i = 0; i < 36/*_scGsmBuffer[i]*/; i++ )
        writeByteToUxRTx( UART_DEBUG, _scGsmBuffer[i] );
    writeStringToUxRTx( UART_DEBUG, "\r\n30 chars in _gsmInputBuffer\r\n" );
    for( i = 0; (i < 30) /*&& (_gsmInputBuffer[i] != 0)*/; i++ )
    {
        if( _gsmInputBuffer[i] == '\r' )
            writeByteToUxRTx( UART_DEBUG, '-' );
        else if( _gsmInputBuffer[i] == '\n' )
            writeByteToUxRTx( UART_DEBUG, '=' );
        else
            writeByteToUxRTx( UART_DEBUG, _gsmInputBuffer[i] );
    }
#endif

    // If disconnected.
    if( !count )
    {
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\nstrstr" );
#endif
        {
            int foundAt = 0;
            foundAt = (int)strstr( (const char *)_gsmInputBuffer, "3\r" );
            if( foundAt)
                putdec_debug( foundAt - (int)_gsmInputBuffer );     // String found at this position.
            else
                putdec_debug( 0 );                                  // String not found.
        }
        if( !memcmp( _gsmInputBuffer, "\r\n3\r", 3 ) )
            return 0;
    }

//        // Reset semaphore.
    memset( _gsmInputBuffer, 0, sizeof( _gsmInputBuffer ));
    memset( _scGsmBuffer,    0, sizeof( _scGsmBuffer    ));
    _semGsmCommandReceived                      = 0;
//        _semGsmInputBuffer                          = maxLengthOfGsmInputBuffer;
    _semGsmPositionPushInputBuffer              = 0;
    _semGsmPositionPushGsmCommandReceivedBuffer = 0;

#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\n===== WAIT =====\r\n" );
#endif

    writeStringToUxRTx( UART_GSM, "AT#SRECV=1," );
//                writeStringToUxRTx( UART_DEBUG, "\r\n=2=" );
	writeStringToUxRTx( UART_GSM, &string[11] );
	if( writeByteToUxRTx( UART_GSM, '\r' ) )
	{
		// Wait for answer from the GSM module.
		while( !_semGsmCommandReceived );
//		while( _semGsmCommandReceived < 2 );
//			writeStringToUxRTx( UART_DEBUG, "\r\n=4=\r\n" );

		DelayMs(1000);
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\n36 char's of data in _scGsmBuffer\r\n" );
		// Extract our received characters.
//		for( i = 0; (i < count) /*&& (i < sizeof( string )-1)*/; i++ )
		for( i = 0; i < 36/*_scGsmBuffer[i]*/; i++ )
		{
//                string[i] = (BYTE)UARTGetDataByte( UART_GSM );
			writeByteToUxRTx( UART_DEBUG, _scGsmBuffer[i] );
//                        if( _scGsmBuffer[i] == '\r' )
//                            writeByteToUxRTx( UART_DEBUG, '\n' );
//                        if( _scGsmBuffer[i] == '\n' )
//                            writeByteToUxRTx( UART_DEBUG, '\r' );
		}
//		writeStringToUxRTx( UART_DEBUG, "\r\n=5=\r\n" );
		writeStringToUxRTx( UART_DEBUG, "\r\n\'count\' chars in _gsmInputBuffer\r\n" );
#endif
		// Output String.
//		for( i = 0; (i < count - 2) /*&& (_gsmInputBuffer[i] != 0)*/; i++ )
		for( i = 0; i < count /*&& (_gsmInputBuffer[i] != 0)*/; i++ )
		{
#ifdef __UART_DEBUG
			writeByteToUxRTx( UART_DEBUG, _gsmInputBuffer[i] );
#endif
			received_data[i] = _gsmInputBuffer[i];

//			if( _gsmInputBuffer[i] == '\r' )
//				writeByteToUxRTx( UART_DEBUG, '\n' );
//			if( _gsmInputBuffer[i] == '\n' )
//				writeByteToUxRTx( UART_DEBUG, '\r' );
		}
#ifdef __UART_DEBUG
		writeStringToUxRTx( UART_DEBUG, "\r\n30 chars in _gsmInputBuffer\r\n" );
		// Output String.
		for( i = 0; (i < 30) /*&& (_gsmInputBuffer[i] != 0)*/; i++ )
		{
			writeByteToUxRTx( UART_DEBUG, _gsmInputBuffer[i] );
//                received_data[i] = _gsmInputBuffer[i];

//                        if( _gsmInputBuffer[i] == '\r' )
//                            writeByteToUxRTx( UART_DEBUG, '\n' );
//                        if( _gsmInputBuffer[i] == '\n' )
//                            writeByteToUxRTx( UART_DEBUG, '\r' );
		}
//            received_data[count] = 0;
		writeStringToUxRTx( UART_DEBUG, "\r\n=6=\r\n" );
#endif
	}
#ifdef __UART_DEBUG
	writeStringToUxRTx( UART_DEBUG, "\r\n===== END =====\r\n" );
#endif

    DelayMs(1000);
    // Reset string to all NULLs.
    memset( _gsmInputBuffer, 0, sizeof( _gsmInputBuffer ));
    memset( _scGsmBuffer,    0, sizeof( _scGsmBuffer    ));
    _semGsmCommandReceived                      = 0;
//        _semGsmInputBuffer                          = maxLengthOfGsmInputBuffer;
    _semGsmPositionPushInputBuffer              = 0;
    _semGsmPositionPushGsmCommandReceivedBuffer = 0;

    return strlen( received_data );
}

int gsmSSEND( char* text_to_send, int int_enable_rx, int int_enable_tx )
{
    int nError = 0;

//    INTEnable(INT_U1RX, INT_DISABLED);
//    INTEnable(INT_U1TX, INT_DISABLED);
    DelayMs(1000);
    clrUARTRxBuffer( UART_GSM );

    // Reset string to all NULLs.
    memset( _gsmInputBuffer, 0, sizeof( _gsmInputBuffer ));
    memset( _scGsmBuffer,    0, sizeof( _scGsmBuffer    ));
    _semGsmCommandReceived                      = 0;
//        _semGsmInputBuffer                          = maxLengthOfGsmInputBuffer;
    _semGsmPositionPushInputBuffer              = 0;
    _semGsmPositionPushGsmCommandReceivedBuffer = 0;

//    if( gsmSendAtCommand( "#SSEND=1\r" ) )
    writeStringToUxRTx( UART_GSM, "AT#SSEND=1\r" );
    {
        DelayMs(2000);
//        if( waitForString( "> ", delay2s ))	/* After this string has received we kan type in our message body */
        if( memchr( _gsmInputBuffer, (int)'>', 16 ) )
        {
//            memset( _gsmInputBuffer, 0, sizeof( _gsmInputBuffer ));
//            memset( _scGsmBuffer,    0, sizeof( _scGsmBuffer    ));
            writeStringToGSM( text_to_send );

#ifdef __UART_DEBUG
			writeStringToUxRTx( UART_DEBUG, "\r\nSending message\r\n" );
#endif
            /* Start sending E-mail by issuing command Ctrl+Z */
            writeByteToUxRTx( UART_GSM, 0x1A );

//#ifdef RESPONSE_FORMAT_RESULT_CODES
//            if( waitForString( "0\r", delay1s ) )
//#else
//            if( waitForString( "OK\r\n", delay1s ) )
//#endif
//                nError = 1;
//            else
//                nError = 0;
            while( !_semGsmCommandReceived );
            DelayMs(1000);
#ifdef __UART_DEBUG
			writeStringToUxRTx( UART_DEBUG, "\r\n30 chars in _gsmInputBuffer\r\n" );
            // Output String.
            {
                int i;
                for( i = 0; (i < 30) /*&& (_gsmInputBuffer[i] != 0)*/; i++ )
                    writeByteToUxRTx( UART_DEBUG, _gsmInputBuffer[i] );
                writeStringToUxRTx( UART_DEBUG, "\r\n36 char's of data in _scGsmBuffer\r\n" );
                for( i = 0; i < 36/*_scGsmBuffer[i]*/; i++ )
                    writeByteToUxRTx( UART_DEBUG, _scGsmBuffer[i] );
            }
#endif

#ifdef RESPONSE_FORMAT_RESULT_CODES_NUMERIC
//            if( !memcmp( _gsmInputBuffer, "0\r", 20 ) )
//            if( !memcmp( _gsmInputBuffer, "0", 20 ) )
            if( (int)strstr( (const char *)_gsmInputBuffer, "0\r" ) < 3 )
#else
            if( waitForString( "OK\r\n", delay1s ) )
#endif
            {
                nError = 1;
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nnError = 1\r\n" );
#endif
            }
            else
            {
                nError = 0;
#ifdef __UART_DEBUG
				writeStringToUxRTx( UART_DEBUG, "\r\nnError = 0\r\n" );
#endif
            }
#ifdef __UART_DEBUG
			writeStringToUxRTx( UART_DEBUG, "\r\nMessage sent\r\n" );
#endif

        };

    }
    // Reset string to all NULLs.
    memset( _gsmInputBuffer, 0, sizeof( _gsmInputBuffer ));
    memset( _scGsmBuffer,    0, sizeof( _scGsmBuffer    ));
    _semGsmCommandReceived                      = 0;
//        _semGsmInputBuffer                          = maxLengthOfGsmInputBuffer;
    _semGsmPositionPushInputBuffer              = 0;
    _semGsmPositionPushGsmCommandReceivedBuffer = 0;

    if( int_enable_rx )
        INTEnable(INT_U1RX, INT_ENABLED);
    if( int_enable_tx )
        INTEnable(INT_U1TX, INT_DISABLED);

    return nError;
}
