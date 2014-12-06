#ifndef GSM_AT_CMD_
#define GSM_AT_CMD_


#define false 0
#define true 1
//Temporary file for GSM definitions. Will end up in the main code eventually.

//#define STR_AT_CMD(CMD) "AT" CMD
//#define STR_AT_READ(CMD) (STR_AT_CMD(CMD) "?")
//#define STR_AT_EXEC(CMD) STR_AT_CMD(CMD) "="
//#define STR_AT_SET(CMD) STR_AT_EXEC(CMD)
//#define STR_AT_TEST(CMD) STR_AT_EXEC(CMD) "?"

#define STR_AT_CMD "AT"
#define STR_AT_EXSTRA_READ "?"
#define STR_AT_EXSTRA_TEST "=?"
#define STR_AT_EXSTRA_SET "="
#define STR_AT_EXSTRA_EXEC ""

#define SUBSTITUTE "\x1A"

enum ResponseCodes {
  OK = 0,
  CONNECT = 1,
  RING = 2,
  NO_CARRIER = 3,
  ERROR = 4,
  CONNECT_1200 = 5,
  NO_DIALTONE = 6,
  BUSY = 7,
  NO_ANSWER = 8,
  CONNECT_2400 = 10,
  CONNECT_4800 = 11,
  CONNECT_9600 = 12,
  CONNECT_14400 = 15,
  CONNECT_1200_75 = 23
};

enum ATCommandTypes {
  AT_CMD_TYPE_READ = 0,
  AT_CMD_TYPE_TEST,
  AT_CMD_TYPE_SET,
  AT_CMD_TYPE_EXEC
};

enum {
  CONTEXT_ACTIVATION_STAT_DEACTIVATE = 0,
  CONTEXT_ACTIVATION_STAT_ACTIVATE = 1
};

enum {
  SSL_ENABLE_SOCKET_DEACTIVATE = 0, /*default*/
  SSL_ENABLE_SOCKET_ACTIVATE = 1
};

#define Report_Mobile_Equipment_Error "+CMEE" //AT+CMEE=0|1|2 enables/disables the report of result code:
                                              //as an indication of an error relating to the +Cxxx commands issued.
                                              //ex: set a wrong pdp context and then activate it, you get a "service not supported" (569) error.

/*******************************************************/
//gsmInit - inits all settings and then inits gprs and checks mobile network status and if the PDP context is activated. If not calls gsmInitGprs using other paramaters.
/*A lot of stuff here: set numeric mode, echo off, set userid, firewall, save profile etc*/
#define Request_Product_Serial_Number_Identification "+CGSN" //IMEI
#define International_Mobile_Subscriber_Identity "+CIMI" //IMSI
#define Socket_Configuration "#SCFG"  //called in gsmInitGprs mostly with default values - #SCFG=1,1,300,90,600,10
#define Define_PDP_Context "+CGDCONT" //gsmInitGprs( "1,\"IP\",\"public.telenor.se\",\"0.0.0.0\",0,0" )
#define Indicator_Control "+CIND" //gsmGetConnectionToMobileNetworkStatus - READ the current status and
                                  //fill AtCind status variable.
                                  //checks service value in the result to figure out if we are connected to
                                  //the mobile network
#define Context_Activation "#SGACT" //gsmEstablishGPRS() READ the state of all the contexts that have been
                                    //defined through the commands +CGDCONT or #GSMCONT
                                    //Activate context by setting #SGACT=1,1
                                    //If context is deactivated then gsmEstablishGPRS() tries setting other
                                    //PDP contexts (by calling gsmInitGprs) with different service providers.
#define fmt_Context_Activation "%d,%d"  //AT#SGACT=<cid>,<stat>[,<userId>,<pwd>]


#define Show_Address "#CGPADDR" //#CGPADDR=1, returns either the IP address for the GSM context
                                //and/or a list of PDP addresses for the specified PDP

#define Firewall_Setup "#FRWL"  //

#define Revision_Identification "+GMR"   //returns the software revision identification.




//"+CIND" query result indicators
//“battchg” - battery charge level
//“signal” - signal quality
//“service” - service availability (1: registered)
//“sounder” - sounder activity
//“message” - message received
//“call” - call in progress
//“roam” - roaming
//“smsfull” - a short message memory storage in the MT has become full (1), or
//“rssi” - received signal (field) strength

//#SGACT query result indicators
//<cidn> - as <cid> before
//<statn> - context status (1: activated)




/*******************************************************/
//gsmConnectToServer()
#define Socket_Dial "#SD" //gsmConnectToServer() "#SD=1,0,<port>,<server>,0,0,0" Last item is connection mode (0) - Henrik calls it command mode.
                          //Default connection mode = 0 (online mode connection), 1 = command mode




/*******************************************************/
//installation_main() - called after gsmInit() - using Command Mode
/*calls firewallinit, echo off, verbose off, checks mobile conn and gprs status then*/
#define Socket_Configuration_Extended "#SCFGEXT" //"#SCFGEXT=1,1,0,2,0,0"
                                                 //We use ListenAutoRsp=0 but instead if we had set ListenAutoRsp flag then after we do #SL the connection is automatically accepted: the CONNECT indication is given and the modem goes into online data mode.
                                                 //We stay in command mode!
#define Send_Data_In_Command_Mode "#SSEND" //sendInstallHello(), calls gsmConnectToServer() and then "#SSEND=1"
#define Socket_Shutdown "#SH" //sendInstallHello() "#SH=1"

#define Socket_Listen_Ring_Indicator "#E2SLRI" //gsmSendAtCommand("#E2SLRI=1150\r");
                                               //Set command enables/disables the Ring Indicator pin response to a Socket Listen connect and, if enabled, the duration of the negative going pulse generated on receipt of connect.

#define Socket_Listen "#SL" //To listen we do #SL=1,1,80\r
                            //and to READ "#SL?\r" to see if we get "#SL: 1" (are we listening now)
                            //SL opens/closes a socket listening for an incoming TCP connection on a specified port.
#define Socket_Status "#SS" //gsmPendingIncomingConnection "#SS=1"

#define URC_SRING "+SRING" //when a TCP connection request comes on the input port, if the sender is not filtered by internal firewall (see #FRWL),
                           //an URC is received: +SRING : <connId> Afterwards we can use #SA to accept the connection or #SH to refuse it.
#define Socket_Accept "#SA" //"#SA=1,1\r"

#define Send_Data_In_Command_Mode "#SSEND" //AT#SSEND=1
#define Receive_Data_In_Command_Mode "#SRECV"



//+CGDCONT?  //see the pdp context, if not set properly then set it with the following command
//+CGDCONT=1,IP,static.telenor.se,0.0.0.0,0,0

//+CIND?  //connected to mobile nw?
//#SGACT? if return 1,0 then set with #SGACT=1,1 to activate PDP Context
//#CGPADDR=1 //shows ip address

//#SD=1,0,80,api.couchsurfing.com,0,0,0
//#SD=1,0,80,api.couchsurfing.com,0,0,1

//#FRWL=1,213.115.113.54,255.255.255.254
//#SL=1,1,3500    -- to accept #SA=1
//#SL=1,1,5678


/*SSL Related Definitions*/
enum CipherSuite {
  TLS_CIPHER_ANY = 0,             //TLS_RSA_WITH_RC4_128_MD5 + TLS_RSA_WITH_RC4_128_SHA + TLS_RSA_WITH_AES_256_CBC_SHA
                                  //Remote server's responsibility to select one of them.
  TLS_RSA_WITH_RC4_128_MD5 = 1,
  TLS_RSA_WITH_RC4_128_SHA = 2,
  TLS_RSA_WITH_AES_256_CBC_SHA = 3
};

enum SSLAuthMode {
  SSL_AUTH_VERIFY_NONE = 0, /*default*/
  SSL_AUTH_VERIFY_SERVER = 1, /*Requires CA certificate to verify server's cert*/
  SSL_AUTH_VERIFY_SERVER_AND_CLIENT = 2 /*Requires Cert, CA Cert and private key*/
};

/*Result of SSLS*/
enum SSLConnectionStatus {
  Socket_Disabled = 0,
  Connection_Closed = 1,
  Connection_Open = 2
};

enum SSLSecDataActions {
  SSL_SEC_DATA_DELETE = 0,
  SSL_SEC_DATA_WRITE = 1,
  SSL_SEC_DATA_READ = 2
};

enum SSLSecDataTypes {
  SSL_SEC_DATA_CERTIFICATE = 0,
  SSL_SEC_DATA_CA_CERT = 1,
  SSL_SEC_DATA_RSA_PRIVATE_KEY = 2
};

enum SSLConnectionCloseTypes {
  SSL_CONNECTION_FREE_DATA = 0, /*Session id and keys are free then AT#SSLFASTD can’t be used to recover the last SSL session [default]*/
  SSL_CONNECTION_SAVE_INIT_DATA = 1 /*SSL session id and keys are saved and a new connection can be made without a complete handshake using AT#SSLFASTD*/
};

enum ConnectionModes {
  CONN_ONLINE_MODE = 0,
  CONN_COMMAND_MODE = 1 /*default*/
};

/****SSL Related Commands****/
#define Enable_Secure_Socket      "#SSLEN" /*AT#SSLEN=1,1 : to enable socket 1 */
#define fmt_Enable_Secure_Socket  "%d,%d" /*AT#SSLEN=<SSId>,<Enable>*/

#define Configure_Sec_Params_SSL_Socket "#SSLSECCFG"
#define fmt_Configure_Sec_Params_SSL_Socket "%d,%d,%d" /*AT#SSLSECCFG= <SSId>,<cipher_suite>,<auth_mode>[,<cert_format>]*/

#define Manage_Security_Data                  "#SSLSECDATA"
#define fmt_Manage_Security_Data_Read_Delete  "%d,%d,%d" /*AT#SSLSECDATA=<SSId>,<Action>,<DataType>[,<size>]*/
#define fmt_Manage_Security_Data_Write        "%d,%d,%d,%d" /*AT#SSLSECDATA=<SSId>,<Action>,<DataType>[,<size>]*/

#define Configure_General_Params_Secure_Socket "#SSLCFG"  //Before opening the SSL socket, several parameters can be configured via this command

#define Open_Secure_Socket_To_Server      "#SSLD"
#define fmt_Open_Secure_Socket_To_Server  "%d,%d,%s,%d,%d"  /*AT#SSLD=<SSId>,<remotePort>,<remoteHost>,<closureType>,<mode>[,<timeout>]*/

#define Fast_Redial_SSL_Socket "#SSLFASTD" //AT#SSLFASTD=<SSId>,<connMode>,<Timeout>
                                          //restore of a previous session avoids full handshake and performs a fast dial,
                                          //which saves time and reduces the TCP payload.

#define Report_Status_SSL_Socket "#SSLS" //AT#SSLS=1



/*
 *
#SSLEN=1,1  //enable socket for ssl
#SSLSECCFG=1,0,0  //socket=1, verify none,
#SSLSECCFG=1,0,1  //socket=1, server authorization = we are client
#SSLD=1,80,api.couchsurfing.org,0,0
#SSLD=1,443,api.couchsurfing.org,0,0
#SSLD=1,443,api.couchsurfing.org,1,0

#SSLSECDATA=<SSId>,<Action>,<DataType>
#SSLSECDATA=1,0,1 //delete CA
#SSLSECDATA=1,1,1 //write CA
#SSLSECDATA=1,2,1 //read CA


#SSLCFG: 1,1,300,90,100,50,0,0,0,0
GET / HTTP/1.1
#SSLFASTD=1,0 //did not work!

#SSLS=1
+CGDCONT=1,IP,static.telenor.se,0.0.0.0,0,0
*
*/

//Had to write a macro since variable length functions cannot call each other
#define gsm_at_set_cmd(base_at_cmd, param_fmt, ...) create_send_at_cmd(AT_CMD_TYPE_SET, base_at_cmd, param_fmt, __VA_ARGS__)

void gsm_at_read_cmd(const char* base_at_cmd);
void gsm_at_test_cmd(const char* base_at_cmd);
void gsm_at_exec_cmd(const char* base_at_cmd);
void gsm_send_at_cmd(const uint8_t* data);
int gsm_at_collect_response_for_long_op(int delay);
int gsm_at_collect_response(void);
const uint8_t* gsm_at_get_response_buf(void);

#endif /* GSM_AT_CMD_ */
