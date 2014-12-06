#include <time.h>

#define	END_OF_FILE	'.'
#define CHECKSUM_DELIMITER '$'
#define EOL_GSM         '\n'
#define BaudRateGSM     9600
//#define BaudRateGSM     115200

//DY
struct connection_state_t;
struct http_request_t;
enum E_GSM_RESPONSE_TYPE{
	GSM_NUMERIC_RESPONSE = 0,
	GSM_TEXT_RESPONSE = 1
};
enum E_GSM_RESPONSE_CODE{
	GSM_RESPONSE_CODE_OK = 0,
	GSM_RESPONSE_CODE_NO_CARRIER = 1
};

extern const char* gsm_responses[][2];

/********************************************************************************************************************
* Sends the escape sequence to the GSM module with appropriate delays.
*
* Arguments:    maxTries           - The maximum tryouts to establish a connection
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern void gsmSendEscapeSequence(void);

/***** Does not funtion *****/
/********************************************************************************************************************
* Suspend a socket and the GSM module is set in command mode.
*
* Arguments:    maxTries           - The maximum tryouts to establish a connection
*               
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmSuspendSocket( int maxTries );
/********************************************************************************************************************
* Alias for gsmSetCommandMode()
********************************************************************************************************************/
//t gsmSetCommandMode( int maxTries ) __attribute__ ((alias("gsmSuspendSocket")));

/********************************************************************************************************************
* Resume socket from command mode and sets GSM module in data mode.
*
* Arguments:    maxTries           - The maximum tryouts to establish a connection
* 			    socketNr           - The number of the socket to resume
********************************************************************************************************************/
extern int gsmResumeSocketConnection( int maxTries, char socketNr );

/////////////// UNTESTED ///////////////
/********************************************************************************************************************
* Closes a suspended socket. Disconnects from remote server.
*
* Requirements: The communication with the GSM module is in command mode.
*				Probably by sgsmSuspendSocket()
*
* Arguments:    socketNr           - The number of the socket to close
*               
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmCloseSocket( int maxTries, char socketNr);


/********************************************************************************************************************
* Sets the GSM module in command mode.
* Use to switch beetwen 
*
* Arguments:    The maximum number of times to tryouts.
********************************************************************************************************************/
extern int gsmSetCommandMode( int maxTries );

/********************************************************************************************************************
* Initialize the GSM module
*
* Arguments:    gsmStatus          - Address to gsmStatus variable (pointer)
*               IMEI               - Address to IMEI variable (pointer)
*               IMSI               - Address to IMSI variable (pointer)
*               AtCind             - Pointer to AtCind vector (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsm_init( unsigned int* gsmStatus, char* IMEI, char* IMSI, char* AtCind  );

/********************************************************************************************************************
* Reset module to its factory default settings.
********************************************************************************************************************/
extern int gsmResetModuleToFactoryDefault( void );

/********************************************************************************************************************
* Configures the result code level.
*
* Arguments:
*               The maximum number of times to tryouts.
*
* Comments:     // 0 - enables result codes (factory default)
                // 1 - every result code is replaced with a <CR>
*               // 2 - disables result codes
********************************************************************************************************************/
extern int gsmEnableResultCodeLevel( char* level, int maxTries );

/********************************************************************************************************************
* Turns echo of AT-command on or off.
*
* Arguments:    Echo On/Off (1/0)   command sent to the device are echoed back to the DTE before the response is given.
*               The maximum number of times to tryouts.
*
* Comments:     // 0 - disables command echo
*               // 1 - enables command echo (factory default) , hence command sent to the device
*               //     are echoed back to the DTE before the response is given.
********************************************************************************************************************/
extern int setCommandEchoMode( int echo, int maxTries );

/********************************************************************************************************************
* Waits for some of the possible 'OK' commands from the GSM module.
*
* Arguments:    *tmp		A pointer to a allready cleared (all Null) array/buffer.
*               length		The length of the array/buffer.
*				waitTime	The maximum ticks of the Core Timer to wait.
*
* Comments:     Depending of how the module is initialized it can return different kind of 'OK'-commands.
*				For example:	Echo/No echo of commands.
*								Result codec in numeric or text.
*								Verbose/Non verbose.
* Return value:	0			Not okej.
*				1			Okej.
********************************************************************************************************************/
extern int gsmWaitForOk(BYTE *tmp, int length, unsigned int waitTime);

/********************************************************************************************************************
* Checks if the GSM module is answering on the 'AT'-command.
*
* Arguments:    --
*
* Comments:     Sends th 'AT'-command and waits for the respons by calling gsmWaitForOk(...);
*
* Return value:	0			Not okej.
*				1			Okej.
********************************************************************************************************************/
extern int gsmCheckIfAnsweringOnATCommand(void);

/********************************************************************************************************************
* Turns on the GSM module.
*
* Arguments:    The maximum number of times to tryouts.
********************************************************************************************************************/
extern int gsmTurnOn( int maxTries );

/********************************************************************************************************************
* Turns off the GSM module with AT-command. This way the module will first disconnect from mobile network, then turn off.
*
* Arguments:    The maximum number of times to tryouts.
********************************************************************************************************************/
extern int gsmSoftTurnOff( int maxTries );

/********************************************************************************************************************
* Turns off the GSM module the hard way. No disconnection from mobile network
*
* Arguments:    The maximum number of times to tryouts.
********************************************************************************************************************/
extern int gsmHardTurnOff( int maxTries );


/********************************************************************************************************************
* Reads the PWRMON pin on the GSM module.
*
* Remarks: Disables the UART : Delay 1s : Read the pin : Enable the UART.
*          This is due to a problem with pinopeation with some PIC32:s.
*
* Arguments:    None.
*
* Return value: The state of PWRMON pin on the GSM module {(0/OFF);(1/ON)}.
********************************************************************************************************************/
extern int gsmReadPwrMon( void );


/********************************************************************************************************************
* Reset the GSM module with AT-command.
* For exact behaviour red the AT-command reference guide from manufacturer.
*
* Arguments:    The maximum number of times to tryouts.
********************************************************************************************************************/
extern int gsmSoftReset( int maxTries );

/********************************************************************************************************************
* Reset the GSM module the hard way.
*
* Arguments:    The maximum number of times to tryouts.
********************************************************************************************************************/
extern int gsmHardReset( void );

extern int gsmShowInitStatus( unsigned int* );

/********************************************************************************************************************
* Check if GSM module is on.
*
* Return value: '1' if on
*               '0' if off
********************************************************************************************************************/
extern int gsmIsOn( void );

// Check if we have a pending incoming call.
extern int gsmPendingIncomingConnection( char* socket );

// Check if we are still listening for incaming calls.
extern int gsmGetListeningStatus( char* socket );

//extern int gsmSetIPR( void );
extern int gsmSetIPR( char *baudRate );

extern int gsmSetFlowControl( void );
extern int gsmSetCommandLevel( void );
extern int gsmSetErrorLevel( void );


/********************************************************************************************************************
* Set the GSM module to update its time according to the networks time, or not.
*
* Arguments:    If the update should occure or not.
*
* Return value: '0' Update of the GSM Module registers could not be done.
*				'1' Automatic Time Zone update.
*               '2' Network Timezone.
*               '3' Automatic Time Zone update and Network Timezone.
********************************************************************************************************************/
extern int gsmUpdateTimeFromGsmNetwork(char updateTimeForNewtwork);

extern int gsmCheckSimCardInserted( char* );
extern int gsmSetTeCharacterSet( void );
extern int gsmSetTcpReassembly( void );
extern int gsmSetUserId( void );
extern int gsmSetUserPassword( void );
extern int gsmSetIcmp( char icmpAcceptance );//( void );
extern int gsmSetFirewallInit( void );
extern int gsmStoreInProfile0( void );

/********************************************************************************************************************
* Sets the GSM module clock
*
* Arguments:    posixString        - The new time in POSIX-format.
*               maxTries           - The maximum number of times to tryouts.
*
* Return value: '1' if successfully set
*               '0' else
********************************************************************************************************************/
extern int gsmSetClock( int posixTime, int maxTries );

/********************************************************************************************************************
* Retrieves the GSM module clock
*
* Arguments:    posixTime          - The retrieved time in POSIX-format.
*               maxTries           - The maximum number of times to tryouts.
*
* Return value: '1' if successfully set
*               '0' else
********************************************************************************************************************/
extern int gsmGetClock( int* posixTime, int maxTries );


/********************************************************************************************************************
* Get the AtCind-string with the GSM module's GSM-status
*
* Arguments:    AtCind             - Pointer to AtCind vector (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmGetConnectionToMobileNetworkStatus( char* );

/********************************************************************************************************************
* Get the AtCind-string with the GSM module's GSM-status
* Internally calling gsmGetConnectionToMobileNetworkStatus()
*
* Arguments:    gsmStatus          - Address to gsmStatus variable (pointer)
*               AtCind             - Pointer to AtCind vector (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmConnectedToMobileNetwork( unsigned int* gsmStatus, char* AtCind );

/********************************************************************************************************************
* Initialize the GSM modules GPRS
*
* Arguments:    gsmStatus          - Address to gsmStatus variable (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmInitGprs( char* );

/********************************************************************************************************************
* Establish a GPRS connection to the mobile network.
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmEstablishGPRS( void );

/********************************************************************************************************************
* Check if a GPRS connection to the mobile network is established.
*
* Arguments:    gsmStatus          - Address to gsmStatus variable (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmCheckGprsEstablished( unsigned int* gsmStatus );

/////////////// UNTESTED ///////////////
/********************************************************************************************************************
* Closes a established GRPS connection from the mobile network.
*
* Arguments:    gsmStatus          - Address to gsmStatus variable (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmCloseGPRS( void );

/********************************************************************************************************************
* Establishing a connection to remote server.
*
* Arguments:    server             - Name or IP number of the server to connect to.
*               port               - The on the server to connect to.
*               maxTries           - The maximum tryouts to establish a connection.
*               commandMode        - Online mode or command mode.
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
//extern int gsmConnectToServer( char* server, char* port, int maxTries );	// HB - Newly added
extern int gsmConnectToServer( char* server, char* port, int maxTries, int commandMode );

/********************************************************************************************************************
* Establishing a connection one of the predifined remote servers.
* Internally calling gsmConnectToServer()
*
* Arguments:    val                - An integer representing the server to connect to. Range [0;2]
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmConnectToServer_Multi( int val );

/********************************************************************************************************************
* Get the IMEI-number of the GSM module
*
* Arguments:    IMEI               - Address to IMEI variable (pointer)
********************************************************************************************************************/
extern void gsm_get_imei( char* );

/********************************************************************************************************************
* Get the IMSI-number of the GSM module
*
* Arguments:    IMSI               - Address to IMEI variable (pointer)
********************************************************************************************************************/
extern void gsm_get_imsi( char* );

/********************************************************************************************************************
* Gets a predefined test file from a predefine server and copy it to the debug port, UART_DEBUG.
*
* Requirements: The communication with the GSM module is in command mode.
*               
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmHttpGet( void );

/////////////// DEPRECATED ///////////////
/********************************************************************************************************************
* Get the AtCind-string with the GSM module's GSM-status
*
* Arguments:    AtCind             - Pointer to AtCind vector (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmAtcindCommand( char* );

/********************************************************************************************************************
* Wait for a string from the GSM module.
*
* Arguments:    string             - String to wait for
*               delay              - Maximum of time to wait for next character
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int waitForString( char* string, unsigned int delay );
//DY
int gsm_wait_for_response(int gsm_response_code, unsigned int delay);
//void SampleAD_Sensors( int AD_T_Array[4]);

/********************************************************************************************************************
* Tryes to download a new firmware/ICP file.
*
* Requirements: Connection established to remote server.
*
* Arguments:    address            - The address in external FLASH memory, SST25, to store the ICP-file
*               fileDirectory      - The directory on server there the file is stored, starts allways with "/"
*               fileName           - The name of the file. In this case the IMEI number
*               httpVersion        - The version of the supported HTTP protocol. List of currently supplyed versions [1.0;1,1]
*               hostString         - The name of the server there file is stored. Might be different then serverToCallString
*               
* Return value: '1' if success
*               '0' else
*
* Function is locking - locks if nothing is received
********************************************************************************************************************/
//TODO remove legacy version - uses checksum rather than crc
int gsm_download_file_http_get_legacy(struct connection_state_t* conn_state);
int gsm_download_file_http_get(struct connection_state_t* conn_state);

//extern int gsmDownloadFileHttpGet_Par( unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString );
int gsmDownloadFileHttpGet_Par( unsigned int address, struct connection_state_t* conn_state, struct http_request_t* httpRequest);
int gsm_download_extended_par(struct connection_state_t* conn_state);

/********************************************************************************************************************
* Tryes to download a new firmware/ICP file.
* Internally calling:              gsmConnectedToMobileNetwork()
*                                  gsmConnectToServer()
*                                  gsmDownloadFileHttpGet()
*
* int gsmDownloadNewFirmwareHttpGet(	unsigned int* gsmStatus, char* AtCind, 
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
* Return value: '1' if success
*               '0' else
*
* Function is locking - internally called functios is locking if downloaded file is empty
********************************************************************************************************************/
//extern int gsm_connect_and_download_firmware(	unsigned int* gsmStatus, char* AtCind,															/* gsmConnectedToMobileNetwork() */
//											char* serverString, char* port, int maxConnectionTries,											/* gsmConnectToServer()          */
//											unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString	/* gsmDownloadFileHttpGet()      */
//										 );

int gsm_connect_and_download_firmware(
    struct connection_state_t* conn_state,
    char* AtCind,
    int maxConnectionTries);

int gsm_connect_and_download_extended_par(struct connection_state_t* conn_state, char* AtCind, int maxConnectionTries, unsigned int address);

/********************************************************************************************************************
* Writes an integer (32bit) to the debug port, UART_DEBUG.
*
* Arguments:    iint               - The integer to write
********************************************************************************************************************/
extern void writeInt( int iint );

/********************************************************************************************************************
* Writes AT-command to the GSM-module.
*
* Arguments:    command            - The command-string to send except the initial "AT"
*
* Return value: '1' if success
*               '0' else
********************************************************************************************************************/
extern int gsmSendAtCommand( char* command );

/********************************************************************************************************************
* Sets the GSM module's e-mail settings.
*
* Return value: '1' if success
*               '0' else
********************************************************************************************************************/
extern int gsmSetEmailConfig( void );

/********************************************************************************************************************
* Check the GSM module's e-mail settings.
*
* Return value: None
********************************************************************************************************************/
extern void gsmCheckEmailConfig( void );

extern void putc_GSM_DEBUG(char character);

/********************************************************************************************************************
* Send a e-mail of ths Node's status to a predefine address.
*
* Arguments:    gsmStatus          - Address to gsmStatus variable (pointer)
*               IMEI               - Address to IMEI variable (pointer)
*               IMSI               - Address to IMSI variable (pointer)
*               AtCind             - Pointer to AtCind vector (pointer)
*
*
* Return value: '1' if success
*               '0' else
********************************************************************************************************************/
extern int gsmEmailStatus( unsigned int* gsmStatus, char* IMEI, char* IMSI, unsigned int* externalFlashMemorySst25Status, int* RF_T_Array,  int* RF_HR_Array, char* RF_N_Array, char* AtCind, unsigned int* S0_pulses  );

extern int cp2r(int cpvalue);
//extern unsigned int SampleS0(void);
extern unsigned int SampleR_Sense(void);
extern void putdec( int n );

/********************************************************************************************************************
* Send a e-mail.
*
* Arguments:    emailAddress       - The address to send to
*               stringToSend       - The string to send
*               IMEI               - Address to IMEI variable (pointer)
*               IMSI               - Address to IMSI variable (pointer)
*               AtCind             - Pointer to AtCind vector (pointer)
*
* Return value: '1' if successfull retreive
*               '0' else
********************************************************************************************************************/
extern int gsmEmailString( char* emailAddress, char* stringToSend, char* IMEI, char* IMSI, char* AtCind );

/********************************************************************************************************************
* Tryes to download a new firmware/ICP file.
* Internally calling:              gsmGprsEstablished()
*                                  gsmGetImei()
*                                  gsmConnectToServer_Multi()
*                                  gsmDownloadFileHttpGet()
*                                  gsmConnectedToMobileNetwork()
*                                  gsmSoftTurnOff()
*                                  gsmHardTurnOff()
*                                  gsmInit()
** 
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
* Return value: '1' if success
*               '0' else
*
* Function is locking - while(1)
********************************************************************************************************************/
extern void gsmDownloadFileHttpGetMain( unsigned int* gsmStatus, char* IMEI, char* IMSI, char* AtCind );
extern void putSignalStrengthBar(char strength);

/* REMOVE */
extern void writeInt( int iint );

// GSM
#define UART_GSM		(UART1)	// New FW
#define UART_GSM_VECTOR         _UART_1_VECTOR
//#define UART_GSM		(UART2) // Old FW
#define nGSM_ON_TRIS		(TRISCbits.TRISC14)
#define	nGSM_ON			(PORTCbits.RC14)
#define GSM_RESET_TRIS		(TRISCbits.TRISC13)
#define	GSM_RESET		(PORTCbits.RC13)
#define nGSM_DISABLE_TRIS	(TRISBbits.TRISB4)
#define	nGSM_DISABLE		(PORTBbits.RB4)
#define nGSM_STAT_TRIS		(TRISDbits.TRISD10)
#define	nGSM_STAT		(PORTDbits.RD10)

//Handshake.
#define nGSM_RTS_TRIS           (TRISFbits.TRISF0)
#define nGSM_RTS                (PORTFbits.RF0)
#define nGSM_CTS_TRIS           (TRISDbits.TRISD9)
#define nGSM_CTS                (PORTDbits.RD9)

//#define IMEI_LENGTH			(17)	//HBB
#define IMEI_LENGTH			(16)
//#define IMSI_LENGTH			(15)	//HBB
#define IMSI_LENGTH			(16)

#include "main.h"

// AW defined pins
//#define HVAC_EN				(PORTDbits.RD5)
//#define HVAC_EN_TRIS		(TRISDbits.TRISD5)

#define nT_VOLTAGE			(PORTFbits.RF3)
#define nT_VOLTAGE_TRIS		(TRISFbits.TRISF3)



enum _GSM_STATUS{	GSM_INITIATED = 0x1,
					GSM_ON = 0x2,
					GSM_IPR_SET = 0x4,
					GSM_FLOW_CONTROL_SET = 0x8,
					GSM_COMMAND_LEVEL_SET = 0x10,
					GSM_ERROR_LEVEL_SET = 0x20,
					//GSM_SIM_CARD_INSERTED = 0x40,
					GSM_TE_CHARACTER_SET = 0x80,
					GSM_TCP_REASSEMBLY_SET = 0x100,
					GSM_USER_ID_SET = 0x200,
					GSM_USER_PASSWORD_SET = 0x400,
					GSM_ICMP_SET = 0x800,
					GSM_STORED_IN_PROFILE_0 = 0x1000,
					GSM_STORED_IN_PROFILE_1 = 0x2000,
					GSM_FIREWALL_INITIATED = 0x4000,
					GSM_CONNECTED_TO_MOBILE_NETWORK = 0x8000,
					GSM_GPRS_INITIATED = 0x10000,
					GSM_GPRS_ESTABLISHED = 0x20000,
					GSM_CONNECTED_TO_SERVER = 0x40000,
					GSM_IMEI = 0x80000,
					GSM_IMSI = 0x100000,
					GSM_SOCKET_CLOSED = 0x200000,
					GSM_GOT_HTTP = 0x400000,
					GSM_ATCIND_COMMAND_RUNNED = 0x800000,
					GSM_WAIT_FOR_STRING = 0x1000000
					};
#define GSM_SIM_CARD_INSERTED GSM_IMSI

#define SHORT_CIRCUIT_AD 		3062325
#define OPEN_CIRCUIT_AD			464854
#define SHORT_CIRCUIT_SENSE_R	0
#define OPEN_CIRCUIT_SENSE_R	180000

//TODO For testing of new HW
int gsmDownloadTest_HttpGet(	unsigned int* gsmStatus, char* AtCind,															/* gsmConnectedToMobileNetwork() */
                                char* serverToCallString, char* port, int maxConnectionTries,											/* gsmConnectToServer()          */
                                unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString	/* gsmDownloadFileHttpGet()      */
                           );
int gsmDownloadTestHttpGet( unsigned int address, char* fileDirectory, char* fileName, char* httpVersion, char* hostString );


#define maxLengthOfGsmInputBuffer   1023    // Have to be (2^n - 1)
// buffer size / data packet size / sensors per minute /number of transmissions per data packet
// = 1024/11/4/3
// = 7.75 data packets in buffer
#define maxLengthOfGsmOutputBuffer  15       // Have to be (2^n - 1) - shortest command is 9 character long.
//#endif /* __ISM_H_ */

#define maxLengthOfGsmCommandBuffer 63       // Have to be (2^n - 1)

extern int  _semGsmNumberOfRowsInputBuffer;
extern int  _semGsmOutputBuffer;
extern BYTE _gsmInputBuffer[maxLengthOfGsmInputBuffer + 1];
extern BYTE _gsmOutputBuffer[maxLengthOfGsmOutputBuffer];
extern int  _semGsmInputBuffer;
extern int  _semGsmPositionPushInputBuffer;
extern int  _semGsmPositionPullInputBuffer;
extern int  _semGsmPositionPushOutputBuffer;
extern int  _semGsmPositionPullOutputBuffer;
extern char _scGsmBuffer[];
extern int  _semGsmCommandReceived;
extern int  _semGsmCommandReceived_2;
extern int  _semGsmPositionPushGsmCommandReceivedBuffer;
extern int  _semGsmPositionPullGsmCommandReceivedBuffer;
extern int  _semGsmATSL;
extern int  _semGsmATSA;

//extern void ActOnGsmCommandReceived( void );  // Acts on commands from GSM module.
//extern int  ActOnGsmCommandReceived( void );  // Acts on commands from GSM module.    // Moved to atc_installation.c/h.
extern int  gsmExtractReceivedData( char* received_data );
extern int  gsmSSEND( char* text_to_send, int int_enable_rx, int int_enable_tx );
