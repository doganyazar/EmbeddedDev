#include <plib.h>
#include <stdint.h>
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "defines.h"
#include "uarts.h"
#include "gsm.h"
#include "delay.h"
#include "common_test_utils.h"
#include "uart_stub.h"
#include "tracer.h"

static test_uart_data_t test_uart_data;

void uart_stub_init(void){
	clear_buffer(test_uart_data.to_gsm_buf, TEST_TO_GSM_BUFF_SIZE);
	clear_buffer(test_uart_data.from_gsm_buf, TEST_FROM_GSM_BUFF_SIZE);
	test_uart_data.to_gsm_index = 0;
	test_uart_data.from_gsm_index = 0;
	test_uart_data.from_gsm_buf_size = 0;
}

void uart_stub_fill_fromgsm(BYTE* data, int size){
	memcpy(test_uart_data.from_gsm_buf + test_uart_data.from_gsm_buf_size, data, size);
	test_uart_data.from_gsm_buf_size += size;

	//Print the last added part
	LOGD("Fromgsm (%d): %s \n", size, test_uart_data.from_gsm_buf + (test_uart_data.from_gsm_buf_size - size));
}

test_uart_data_t* uart_stub_get_test_data(void){
	return &test_uart_data;
}


int comm_uart_is_received_data_available( int UARTid ){
	if (UARTid == UART_GSM){
		if (test_uart_data.from_gsm_index < test_uart_data.from_gsm_buf_size){
			return 1;
		} else {
			return 0;
		}
	}

	return UARTReceivedDataIsAvailable(UARTid);
}

//TODO Why is this copy of recUART? The production code needs refactoring maybe and remove the call to this function?
char comm_uart_get_data_byte( int UARTid ){
  char result = 0;

  if (UARTid == UART_GSM){
    if (test_uart_data.from_gsm_index < test_uart_data.from_gsm_buf_size ){
      result = test_uart_data.from_gsm_buf[test_uart_data.from_gsm_index++];
    }
  }

  return result;
}

//TODO stub this if needed?
void comm_uart_send_data_byte( int UARTid, char byteToWrite ){
	return UARTSendDataByte(UARTid, byteToWrite);
}


inline void __attribute__((always_inline)) UARTClrBits ( UART_MODULE id, UART_CONFIGURATION flags )
{
//	//UART_REGS *uart;
//
//    //uart = (UART_REGS *)uartReg[id];
//
//	uartReg[id]->sta.clr = (flags);	// Defined in <peripheral/uart.h>
//    //uart->sta.clr = (flags) >> 16;
}

inline void __attribute__((always_inline)) clrUARTRxBuffer( UART_MODULE UARTid )
{
	//UART_OVERRUN_ERROR
	UARTClrBits( UARTid, UART_OVERRUN_ERROR );
}

//inline int __attribute__((always_inline)) recUART( UART_MODULE UART_id, char* c ) //REMOVED
inline int __attribute__((always_inline)) recUART( int UART_id, BYTE* c ) //ADDED
{
	int result = 1;

	if (UART_id == UART_GSM){
		if (test_uart_data.from_gsm_index < test_uart_data.from_gsm_buf_size ){
			*c = test_uart_data.from_gsm_buf[test_uart_data.from_gsm_index++];
		} else {
			result = 0;
		}
	}

	return result;
}

//inline int __attribute__((always_inline)) recUART_timed( int UART_id, BYTE* c, int end_time ) //ADDED
//{
//	//int delay1ms = 0x13880;		// 1ms
//	//int delay1s = 0x34C4B40;	// 3s
//	//int delay1S = 0x1312D00;
//
//	unsigned int savedCoreTime;
//    char         received_status;
//
//	/* Save value of core timer */
//	savedCoreTime = ReadCoreTimer();
//
//	//WriteTimer23( 0 );
//	WriteCoreTimer( 0 );
////	do
////	{
////	//while( !UARTReceivedDataIsAvailable( UART_id ) );
////		if( UARTReceivedDataIsAvailable( UART_id ) )
////		{
////			*c = UARTGetDataByte( UART_id );
////			WriteCoreTimer( savedCoreTime );
////			return 1;
////		}
////	}while( ReadCoreTimer() < delay1s );
//
//	while( !UARTReceivedDataIsAvailable( UART_id ) && (ReadCoreTimer() < end_time) );
//	if( UARTReceivedDataIsAvailable( UART_id ) )
//	{
//		*c = UARTGetDataByte( UART_id );
//		received_status = 1;
//	}
//	else
//		received_status = 0;
//
//
//	/* Restore value in core timer */
//	WriteCoreTimer( savedCoreTime );
//
//	//*c = 'R';
//	//return UARTGetDataByte( UARTid );
////        return 0;
//        return received_status;
//}
//

inline int __attribute__((always_inline)) writeByteToUxRTxDebug(BYTE byteToWrite){
	unsigned int savedCoreTime;

	/* Save value of core timer */
	savedCoreTime = ReadCoreTimer();

	while( (uartReg[UART_DEBUG]->sta.reg & _U1STA_UTXBF_MASK) && (ReadCoreTimer() < delay1s) );
	WriteCoreTimer( 0 );
	do
	{
		if(UARTTransmitterIsReady( UART_DEBUG ))
		{
			comm_uart_send_data_byte( UART_DEBUG, byteToWrite );
			return 1;
		}
	}while( ReadCoreTimer() < delay1s );

	/* Restore value in core timer */
	WriteCoreTimer( savedCoreTime );

	return 0;
}

inline int __attribute__((always_inline)) writeByteToUxRTx( UART_MODULE UARTid, BYTE byteToWrite ){
	if (UARTid == UART_DEBUG){
		return writeByteToUxRTxDebug(byteToWrite);
	}

	if (UARTid == UART_GSM){
		test_uart_data.to_gsm_buf[test_uart_data.to_gsm_index++] = byteToWrite;
	}

	return 1;
}

//inline int __attribute__((always_inline)) writeStringToUxRTxDebug(char* string ){
//	int i;
//
//	for( i = 0; i < strlen( string ); i = i + 1 )
//		if( !writeByteToUxRTx( UART_DEBUG, string[i] ) )
//			return 0;
//
//	return 1;
//}

inline int __attribute__((always_inline)) writeStringToUxRTx(UART_MODULE UARTid, char* string) {
  //	if (UARTid == UART_DEBUG){
  //		return writeStringToUxRTxDebug(string);
  //	}

  int i;

  for (i = 0; i < strlen(string); i = i + 1) {
    if (!writeByteToUxRTx(UARTid, (BYTE) string[i])) {
      return 0;
    }
  }

//  //DY - remove later
//  if (UARTid == UART_GSM) {
//    uint8_t fake_data[50];
//    LOGD("X: %s", string);
//
//    if (strstr(string, "+IPR?\r")) {
//      strcpy((char*) fake_data, "+IPR: 96000\r0\r");
//      uart_stub_fill_fromgsm((BYTE *) fake_data, strlen((char*)fake_data));
//    } else if (strchr(string, '\r')) {
//      strcpy((char*) fake_data, "0\r");
//      uart_stub_fill_fromgsm((BYTE *) fake_data, strlen((char*)fake_data));
//    } else {
//
//    }
//  }

  return 1;
}

int writeStringToGSM(char* string) {
  return writeStringToUxRTx( UART_GSM, string);
}
