/*
 * uart_stub.h
 *
 *  Created on: Aug 9, 2013
 *      Author: dogan
 */

#ifndef UART_STUB_H_
#define UART_STUB_H_

#define TEST_TO_GSM_BUFF_SIZE 1000
#define TEST_FROM_GSM_BUFF_SIZE 1000
typedef struct {
	char to_gsm_buf[TEST_TO_GSM_BUFF_SIZE];
	int to_gsm_index;
	char from_gsm_buf[TEST_FROM_GSM_BUFF_SIZE];
	int from_gsm_index;
	int from_gsm_buf_size;
} test_uart_data_t;

void uart_stub_init(void);
test_uart_data_t* uart_stub_get_test_data(void);
void uart_stub_fill_fromgsm(BYTE* data, int size);

#endif /* UART_STUB_H_ */
