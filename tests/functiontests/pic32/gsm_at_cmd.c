#include <stdint.h>
#include "gsm_at_cmd.h"
#include "tracer.h"
#include "delay.h"
#include "comm.h"
#include "gsm.h"
#include "uarts.h"
#include "timer.h"

//TODO DY check buffer boundaries and find a good solution to handle the situation when there is a problem.
//TODO buffer sizes - find a better solution for multiple buffers

#define AT_BUF_SIZE 200
#define DEFAULT_GSM_AT_CMD_WAIT_DELAY delay1s

static uint8_t at_buf[AT_BUF_SIZE];
static uint8_t* at_ptr;

static uint8_t at_response_buf[2048]; //set it since max sec data (certificate) is 2047 bytes
static int at_response_buf_index = 0;

void gsm_at_init(void) {
  at_ptr = at_buf;
  return;
}

int at_update_buf_ptr(int num) {
  if (num > 0) {
    at_ptr += num;

    return true;
  }

  return false;
}

void at_any_cmd(int type, const char* at_cmd) {
  const char* extra = NULL;

  gsm_at_init();

  switch (type) {
  case AT_CMD_TYPE_READ:
    extra = STR_AT_EXSTRA_READ; break;
  case AT_CMD_TYPE_TEST:
    extra = STR_AT_EXSTRA_TEST; break;
  case AT_CMD_TYPE_SET:
    extra = STR_AT_EXSTRA_SET; break;
  case AT_CMD_TYPE_EXEC:
    extra = STR_AT_EXSTRA_EXEC; break;
  default:
    extra = STR_AT_EXSTRA_READ; break;
    break;
  }

  at_update_buf_ptr(snprintf((char *)at_ptr, 50, "%s%s%s", STR_AT_CMD, at_cmd, extra));

}

//TODO this can end up being the ultimate function to wait for data from UART
int wait_for_at_answer(int delay) {
  WriteCoreTimer(0);

  //Loop until data is available or timeout
  while (!comm_uart_is_received_data_available(UART_GSM)
      && ReadCoreTimer() < delay)
    ;

  return comm_uart_is_received_data_available(UART_GSM);
}

//TODO this can end up being the ultimate function to receive data from UART
int at_receive_uart_data(void) {
  BYTE c = 0;

  at_response_buf_index = 0;
  at_response_buf[0] = 0;

  WriteCoreTimer(0);
  //Wait 1 second until all data is available.
  do {
    while (comm_uart_is_received_data_available(UART_GSM)) {
      if (recUART(UART_GSM, &c)) {
        //TODO buffer size check!
        at_response_buf[at_response_buf_index++] = c;
      }
    }
  } while (ReadCoreTimer() < delay3s);

  at_response_buf[at_response_buf_index] = 0; //safety measure

  LOGD("AT Response (Size: %d): %s", at_response_buf_index, at_response_buf);

  return at_response_buf_index;
}

void send_at_cmd_from_buf() {
  gsm_send_at_cmd(at_buf);
}

//Had to write a macro since variable length functions cannot call each other
//#define gsm_at_send_set(base_at_cmd, param_fmt, ...) gsm_at_send(AT_CMD_TYPE_SET, base_at_cmd, param_fmt, __VA_ARGS__)

void create_send_at_cmd(int cmd_type, const char* base_at_cmd, const char* param_fmt, ...) {

  at_any_cmd(cmd_type, base_at_cmd);

  va_list argptr;
  va_start(argptr, param_fmt);
  vsnprintf((char *)at_ptr, 100, param_fmt, argptr);
  va_end(argptr);

  send_at_cmd_from_buf();
}

/*Api*/

void gsm_at_read_cmd(const char* base_at_cmd) {
  create_send_at_cmd(AT_CMD_TYPE_READ, base_at_cmd, "");
}

void gsm_at_test_cmd(const char* base_at_cmd) {
  create_send_at_cmd(AT_CMD_TYPE_TEST, base_at_cmd, "");
}

void gsm_at_exec_cmd(const char* base_at_cmd) {
  create_send_at_cmd(AT_CMD_TYPE_EXEC, base_at_cmd, "");
}

void gsm_send_at_cmd(const uint8_t* data) {
  LOGD("Cmd: %s", data);

  writeStringToUxRTx(UART_GSM, (char *)data);
  writeStringToUxRTx(UART_GSM, "\r");
}

int gsm_at_collect_response_for_long_op(int delay) {
  int num_of_bytes_read = 0;
  if (wait_for_at_answer(delay)) {
    num_of_bytes_read = at_receive_uart_data();
  }

  return num_of_bytes_read;
}

int gsm_at_collect_response(void) {
  return gsm_at_collect_response_for_long_op(DEFAULT_GSM_AT_CMD_WAIT_DELAY);
}

const uint8_t* gsm_at_get_response_buf(void) {
  return at_response_buf;
}
