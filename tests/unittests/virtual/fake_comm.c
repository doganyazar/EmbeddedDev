/***
 * Excerpted from "Test-Driven Development for Embedded C",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material,
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose.
 * Visit http://www.pragmaticprogrammer.com/titles/jgade for more book information.
***/
#include <stdio.h>
#include <string.h>
#include "fake_comm.h"
#include "main.h" //For MAXDATASIZE

static char send_buf[100];
static int buf_index = 0;

static const char* recv_buf;
static int recv_buf_index = 0;
static int recv_buf_size = 0;

#define MAXDATASIZE 1024

//Need these extern vars since they are defined in Comm_TCP or else linking fails.
int			sockfd;
char		buf[MAXDATASIZE];

void Fake_Comm_Create(void)
{
	send_buf[0]=0;
	buf_index = 0;
	recv_buf = NULL;
	recv_buf_index = 0;
	recv_buf_size = 0;
}

void Fake_Comm_Destroy(void)
{
}

char* Fake_Comm_Get_Buffer(void){
	return send_buf;
}

int Fake_Comm_Get_Buffer_Size(void){
	return buf_index;
}

void Fake_Comm_Set_Rec_Buffer(const char* buf, int size){
	recv_buf = buf;
	recv_buf_index = 0;
	recv_buf_size = size;
}

int SendByte(BYTE byteToWrite){
	send_buf[buf_index++] = byteToWrite;
	return 1; //Success
}

inline void __attribute__((always_inline)) clrUARTRxBuffer( int UARTid ){
	printf("Not Fake Implemented \n");
}

inline int __attribute__((always_inline)) recUART( int UARTid, char* c ){
    if (recv_buf_index < recv_buf_size) {
        *c = recv_buf[recv_buf_index++];
        return 1;
    } else {
        return 0;
    }
}

int SendString(const char* string){
	int i;

	for( i = 0; i < strlen( string ); i = i + 1 )
		if( !SendByte(string[i]))
			return 0;

	return 1;
}

int SendByteToIsm(BYTE byte){
	printf("Not Fake Implemented \n");
	return 1;
}

int writeStringToGSM( char* string ){
	printf("Not Fake Implemented \n");
	return 1;
}


int comm_uart_is_received_data_available( int UARTid )
{
	printf("Not Fake Implemented \n");
	return 1;
}

char comm_uart_get_data_byte( int UARTid )
{
	printf("Not Fake Implemented \n");
	return 0;
}

int comm_uart_send_data_byte( int UARTid, char byteToWrite )
{
	printf("Not Fake Implemented \n");
	return 1;
}

int comm_tcp_recv_data(void) {
    return 1;
}

int comm_tcp_connect_to_server(const char* server) {
    return 1;
}
