/*
 * auto_test.c
 *
 *  Created on: Jul 15, 2013
 *      Author: dogan
 */

//#include <peripheral/wdt.h> //Needed for disabling watchdog
#include "test.h"
#include "auto_test.h"
#include "GenericTypeDefs.h"
#include "gsm.h"
#include "tracer.h"
#include "server_comm.h"

#include "common_test_utils.h"
#include "pic32_test_defs.h"

#include "spis.h"
#include "SST25.h"
#include "SST25VF064C.h"
#include "memorymap.h"
#include "delay.h"

#include "gsm_at_cmd.h"

//LED BLINK
//        while(1){
//        	LED_SERVICE_INV;
//        	DelayMs(500);
//        }

char AtCind[18];
char IMEI[16];
char IMSI[16];

void gsm_init_test() {
  unsigned int gsmStatus = 0;
  unsigned int ismStatus = 0;

//    unsigned int externalFlashMemorySst25Status;

  memset(AtCind, 0, 18);	// Null at end
  memset(IMEI, 0, 16);	// Null at end
  memset(IMSI, 0, 16);	// Null at end

  LOGD("QQQQQQQ");
  LOGD("Test Start");

  TRACE_CALL_ENTER()
  ;

  LOGD("Test GSM ON/OFF");
  if (gsmReadPwrMon()) {
    BYTES_EQUAL(1, gsmHardTurnOff(1));
  } else {
    BYTES_EQUAL(2, gsmHardTurnOff(1));
  }
  BYTES_EQUAL(1, gsmTurnOn(1));

  LOGD("Test GSM Init");
  BYTES_EQUAL(1, gsm_init(&gsmStatus, IMEI, IMSI, AtCind));

//	testfunc( &externalFlashMemorySst25Status );
}

void http_basic_par_test() {

  /*
   Original:
   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 3A 98 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 c5 00 00 00 00 00 00 fd 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 10 00 //64 bytes
   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 10 00

   Downloaded:
   0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  3a 98  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0 c5  0  0  0  0  0  0 fd  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  10 0
   0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  10  0
   */
  char* IMEI_par = "351777047368145.par";

  connection_state_t conn_state;

  extern char AtCind[];
  LOGD("ATCind:");
  print_buffer_hex(AtCind, 23);

  conn_state.request.server = "hem.bredband.net";
  conn_state.request.port = "80";
  conn_state.request.fileDirectory = "/henrikborg/";
  conn_state.request.fileName = IMEI_par;
  conn_state.request.httpVersion = "1.1";
  LONGS_EQUAL(1, http_download_extended_par(&conn_state));

  LOGD("ATCind:");
  print_buffer_hex(AtCind, 23);
  print_buffer(AtCind, 23);
  LOGD("Buffers:");
  print_buffer_hex(conn_state.data_store.par_file_string, 64);
  print_buffer_hex(conn_state.data_store.par_file_mask, 64);
}

void http_extendedpar_test(const char* filename) {
  TRACE_CALL_ENTER()
  ;

  const char* IMEI_par = filename;

  connection_state_t conn_state;

//    uart_stub_init();
  server_comm_init_connection(&conn_state);

  conn_state.request.server = "hem.bredband.net";
  conn_state.request.port = "80";
  conn_state.request.fileDirectory = "/henrikborg/";
  conn_state.request.fileName = IMEI_par;
  conn_state.request.httpVersion = "1.1";

  LONGS_EQUAL(1, http_download_extended_par(&conn_state));

  uint8_t output[64];
  readStringFromSST25(0x401B8100, output, 25);
  LOGD("Flash Char Write:");
  print_buffer(output, 24);

  readStringFromSST25(0x401B8200, output, 20);
  LOGD("Flash Hex:");
  print_buffer_hex(output, 20);
}


//void dummy_gsm_test() {
//  unsigned int gsmStatus = 0;
//  unsigned int ismStatus = 0;
//
////    unsigned int externalFlashMemorySst25Status;
//
//  memset(AtCind, 0, 18);  // Null at end
//  memset(IMEI, 0, 16);  // Null at end
//  memset(IMSI, 0, 16);  // Null at end
//
//
//  TRACE_CALL_ENTER();
//
//  LOGD("Test GSM ON/OFF");
//  if (gsmReadPwrMon()) {
//    BYTES_EQUAL(1, gsmHardTurnOff(1));
//  } else {
//    BYTES_EQUAL(2, gsmHardTurnOff(1));
//  }
//  BYTES_EQUAL(1, gsmTurnOn(1));
//
//
//
//  LOGD("Test GSM Init");
//  BYTES_EQUAL(1, gsm_init(&gsmStatus, IMEI, IMSI, AtCind));
//
////  testfunc( &externalFlashMemorySst25Status );
//}


const char* Starfield_Class_2_CA =
"-----BEGIN CERTIFICATE-----\n" \
"MIIEDzCCAvegAwIBAgIBADANBgkqhkiG9w0BAQUFADBoMQswCQYDVQQGEwJVUzEl\n" \
"MCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMp\n" \
"U3RhcmZpZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMDQw\n" \
"NjI5MTczOTE2WhcNMzQwNjI5MTczOTE2WjBoMQswCQYDVQQGEwJVUzElMCMGA1UE\n" \
"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMpU3RhcmZp\n" \
"ZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwggEgMA0GCSqGSIb3\n" \
"DQEBAQUAA4IBDQAwggEIAoIBAQC3Msj+6XGmBIWtDBFk385N78gDGIc/oav7PKaf\n" \
"8MOh2tTYbitTkPskpD6E8J7oX+zlJ0T1KKY/e97gKvDIr1MvnsoFAZMej2YcOadN\n" \
"+lq2cwQlZut3f+dZxkqZJRRU6ybH838Z1TBwj6+wRir/resp7defqgSHo9T5iaU0\n" \
"X9tDkYI22WY8sbi5gv2cOj4QyDvvBmVmepsZGD3/cVE8MC5fvj13c7JdBmzDI1aa\n" \
"K4UmkhynArPkPw2vCHmCuDY96pzTNbO8acr1zJ3o/WSNF4Azbl5KXZnJHoe0nRrA\n" \
"1W4TNSNe35tfPe/W93bC6j67eA0cQmdrBNj41tpvi/JEoAGrAgEDo4HFMIHCMB0G\n" \
"A1UdDgQWBBS/X7fRzt0fhvRbVazc1xDCDqmI5zCBkgYDVR0jBIGKMIGHgBS/X7fR\n" \
"zt0fhvRbVazc1xDCDqmI56FspGowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0\n" \
"YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBD\n" \
"bGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8w\n" \
"DQYJKoZIhvcNAQEFBQADggEBAAWdP4id0ckaVaGsafPzWdqbAYcaT1epoXkJKtv3\n" \
"L7IezMdeatiDh6GX70k1PncGQVhiv45YuApnP+yz3SFmH8lU+nLMPUxA2IGvd56D\n" \
"eruix/U0F47ZEUD0/CwqTRV/p2JdLiXTAAsgGh1o+Re49L2L7ShZ3U0WixeDyLJl\n" \
"xy16paq8U4Zt3VekyvggQQto8PT7dL5WXXp59fkdheMtlb71cZBDzI0fmgAKhynp\n" \
"VSJYACPq4xJDKVtHCN2MQWplBqjlIapBtJUhlbl90TSrE9atvNziPTnNvT51cKEY\n" \
"WQPJIrSPnNVeKtelttQKbfi3QBFGmh95DmK/D5fs4C8fF5Q=\n" \
"-----END CERTIFICATE-----\n";


void dodo_test_func(void) {
  int i, i2, j;
  char menuState[] = "\0\0";
  char AtCind[18];
  char IMEI[16];
  char IMSI[16];
  unsigned int gsmStatus = 0;
  unsigned int ismStatus = 0;

  char delimiter = '\r';

  const char* get1 = "GET / HTTP/1.1\n"\
                    "Host: api.couchsurfing.org\n"\
                    "\n";

  memset(AtCind, 0, 18);  // Null at end
  memset(IMEI, 0, 16);  // Null at end
  memset(IMSI, 0, 16);  // Null at end


  char c;
  char input[50];
  int input_index = 0;
  char gsm_command[50];
  int send_as_command = 1;

  LOGD("Ready!");

  if (gsmTurnOn(1)) {
     LOGD("GSM module is On");
  }

  // Clear UART buffer.
  clrUARTRxBuffer( UART_DEBUG);

  while (1) {
//    // Simple delay 0.5s
//    DelayMs(100);

    if (comm_uart_is_received_data_available(UART_DEBUG)) {
      c = comm_uart_get_data_byte(UART_DEBUG);

      input[input_index++] = c;
      if (c == delimiter) {
        input[input_index] = 0;
        LOGD("> %s", input);

        if (!strncmp(input, "aaa", 3)) {
          send_as_command = (++send_as_command) % 2;
          if (send_as_command) {
            LOGD("AT ON");
          } else {
            LOGD("AT OFF");
          }
        } else if (!strncmp(input, "ddd", 3)) {
          if (delimiter == '\r') {
            delimiter = '!';
          } else {
            delimiter = '\r';
          }
          LOGD("Delimiter=%c", delimiter);
        } else if (!strncmp(input, "cmd1", 4)) {
          strcpy(gsm_command, get1);
          writeStringToUxRTx(UART_GSM, gsm_command);
        } else if (!strncmp(input, "cmd2", 4)) {
          sprintf(gsm_command, "AT#SSLSECDATA=1,1,1,%d\r\n",strlen(Starfield_Class_2_CA));
          writeStringToUxRTx(UART_GSM, gsm_command);
        } else if (!strncmp(input, "cmd3", 4)) {
          //26: SUB char
          writeStringToUxRTx(UART_GSM, Starfield_Class_2_CA);
          char temp[2] = {26, 0};
          writeStringToUxRTx(UART_GSM, temp);
        } else {
          //Execute command here
          if (send_as_command) {
            strcpy(gsm_command, "AT");
          }
          strcat(gsm_command, input);

          writeStringToUxRTx(UART_GSM, gsm_command);
        }
        input_index = 0;
        input[0] = 0;
      }

    }

    while (comm_uart_is_received_data_available( UART_GSM)) {
      c = comm_uart_get_data_byte( UART_GSM);
      writeByteToUxRTx(UART_DEBUG, c);
    }
  }
}

//TODO
//1) find a good way of waiting for GSM data, now it waits 3 sec in between
//2) find a good way of parsing the data coming from GSM. We expect ok most of the time anyway.
//3) find out why sometimes some commands fail. Ex: #SGACT=1,1 fails even when context was disabled. IT maybe because GSM module is not read after the previous command. May need a delay in between.
void gsm_ssl_test(void) {
  LOGD("GSM SSL TEST");

  if (gsmTurnOn(1)) {
    LOGD("GSM module is On");
  }

  int context_id = 1;
  int socket_id = 1;


  gsm_at_read_cmd(Context_Activation);
  gsm_at_collect_response();

  //#SGACT returns error if already enabled
  gsm_at_set_cmd(Context_Activation, fmt_Context_Activation, context_id, CONTEXT_ACTIVATION_STAT_ACTIVATE);
  gsm_at_collect_response();

  //SSLEN returns error if already enabled

//  gsm_at_set_cmd(Enable_Secure_Socket, fmt_Enable_Secure_Socket, socket_id, SSL_ENABLE_SOCKET_DEACTIVATE);
//  gsm_at_collect_response();

  gsm_at_exec_cmd(Request_Product_Serial_Number_Identification); //IMEI
  gsm_at_collect_response();

  gsm_at_exec_cmd(International_Mobile_Subscriber_Identity);  //IMSI
  gsm_at_collect_response();


  gsm_at_set_cmd(Enable_Secure_Socket, fmt_Enable_Secure_Socket, socket_id, SSL_ENABLE_SOCKET_ACTIVATE);
  gsm_at_collect_response();


  gsm_at_set_cmd(Manage_Security_Data, fmt_Manage_Security_Data_Read_Delete, socket_id, SSL_SEC_DATA_DELETE, SSL_SEC_DATA_CA_CERT);
  gsm_at_collect_response();

  gsm_at_set_cmd(Manage_Security_Data, fmt_Manage_Security_Data_Read_Delete, socket_id, SSL_SEC_DATA_READ, SSL_SEC_DATA_CA_CERT);
  gsm_at_collect_response();

  gsm_at_set_cmd(Manage_Security_Data, fmt_Manage_Security_Data_Write, socket_id, SSL_SEC_DATA_WRITE, SSL_SEC_DATA_CA_CERT, strlen(Starfield_Class_2_CA));
  gsm_at_collect_response();

  writeStringToUxRTx(UART_GSM, Starfield_Class_2_CA);
  writeStringToUxRTx(UART_GSM, SUBSTITUTE);

  gsm_at_collect_response();

  gsm_at_set_cmd(Manage_Security_Data, fmt_Manage_Security_Data_Read_Delete, socket_id, SSL_SEC_DATA_READ, SSL_SEC_DATA_CA_CERT);
  gsm_at_collect_response();

  gsm_at_set_cmd(Configure_Sec_Params_SSL_Socket, fmt_Configure_Sec_Params_SSL_Socket, socket_id, TLS_CIPHER_ANY, SSL_AUTH_VERIFY_SERVER);
  gsm_at_collect_response();

  gsm_at_set_cmd(
      Open_Secure_Socket_To_Server, fmt_Open_Secure_Socket_To_Server,
      socket_id, 443, "api.couchsurfing.org", SSL_CONNECTION_SAVE_INIT_DATA, CONN_ONLINE_MODE);
  gsm_at_collect_response_for_long_op(delay50s);

  writeStringToUxRTx(UART_GSM, "GET / HTTP/1.1\r\nHost: api.couchsurfing.org\r\n\r\n");
  gsm_at_collect_response_for_long_op(delay15s);

}

void auto_test() {

  tracer_init();

  unsigned int externalFlashMemorySst25Status;

  printf("Auto Test \n");

//  gsm_init_test();
////	http_basic_par_test();
//  http_extendedpar_test("extended.par");

//  install();

//  gsm_get_imei(IMEI);
//  LOGD("IMEI: %s", IMEI);

//  dummy_gsm_test();

//  testfunc(&externalFlashMemorySst25Status);

  gsm_ssl_test();
  dodo_test_func();

}
