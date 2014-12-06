#include <stdio.h>
#include "common_test_utils.h"
#include "pic32_test_defs.h"
#include "unit_test_main.h"
#include "tracer.h"
#include "atc_platform_init.h"
#include "gsm.h"
#include "server_comm.h"
#include "HardwareProfile.h"
#include "delay.h"
#include "uart_stub.h"
#include "uarts.h"
#include "timer.h"

#include "spis.h"
#include "SST25.h"
#include "SST25VF064C.h"
#include "memorymap.h"

#include "crc_sw.h"
#include "crc_hw.h"

#include "atc_hvac_r_ladder.h"

#include "ism.h"

const char* firmware_str =
":020000040000fa\n" \
":020000041fc01b\n" \
":042ff400d979f8ff90\n" \
":020000040000fa\n" \
":020000041fc01b\n" \
":042ff800dbee7fff8e\n" \
":020000040000fa\n" \
":020000041fc01b\n" \
":042ffc00feffff7f56\n" \
":020000040000fa\n" \
":020000041d00dd\n" \
":1061000000bd1a3c10615a2708004003000000003f\n" \
":1061100000601a40c0045a7f0500401300000000d0\n";

const char* firmware_checksum_str = "$00003eff."; //checksum adds all characters including newlines
const char* firmware_crc_32_str = "$5377d360.";
uint32_t firmware_crc_32 = 0x5377d360;

const char* server = "hem.bredband.net";
const char* port = "80";
const char* fileDirectory = "/henrikborg/";
const char* fileName = "extended.par";
const char* httpVersion = "1.1";
const char* hostString = "hem.bredband.net"; //TODO will be removed later.

const char* crc_str = "3e96dd83^";
const char* extended_par =
    "000000000000000000000000000000003A980000000000000000000000000000000000c5000000000000fd000000000000000000000000000000000000011000\n" \
"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011000.\n" \
"// Testing file for extended .par-file.\n" \
"@401B8100 abb2 cce3    0123 4567   89ab cdef   // These bytes will NOT be converted\n" \
"\n" \
"401B8200 abb2 cce3 0123 4567   89ab cdef   4567 89ab   cdef 0123   // These bytes will be converted\n" \
"$";

const char* extended_par_http_headers =
    "HTTP/1.1 200 OK\n" \
"Date: Tue, 13 Aug 2013 10:53:28 GMT\n" \
"Server: Apache/2.2.3 (CentOS)\n" \
"Last-Modified: Fri, 02 Aug 2013 10:42:50 GMT\n" \
"ETag: \"229cc0-103-4e2f49ebe1e80\"\n" \
"Accept-Ranges: bytes\n" \
"Content-Length: 259\n" \
"Content-Type: text/plain\n" \
"\n";

uint8_t expected_char[] = "abb2cce30123456789abcdef";
uint8_t expected_hex[] = { 0xab, 0xb2, 0xcc, 0xe3, 0x1, 0x23, 0x45, 0x67, 0x89,
    0xab, 0xcd, 0xef, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x1, 0x23 };

uint8_t expected_par_str[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3a, 0x98, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc5, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xfd, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x10, 0x0 };

uint8_t expected_par_mask[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x10, 0x0 };

const char* expected_gsm_request =
    "GET /henrikborg/extended.par HTTP/1.1 \r\nHost: hem.bredband.net\r\n\r\n";
int expected_gsm_request_length = 0;

//TODO make the tests to run automatically. Some macro that would add them to a predefined linked list for example.

///* Originally put it in tracer_pic32.c file but somehow it does not work for unit tests, so defined it here as well.
// * Needed to make printf work.S
// */
void _mon_putc(char c) {
  // printf() passes each character to write to _mon_putc(),
  //which in turn passes each character to a custom output function
  writeByteToUxRTx(UART_DEBUG, (BYTE) c);
}

test_uart_data_t* t_uart;

connection_state_t conn_state;


void test_init_connection(connection_state_t* conn_state){

  server_comm_init_connection(conn_state);

  conn_state->request.server = (char*)server;
  conn_state->request.port = (char*)port;
  conn_state->request.fileDirectory = (char*)fileDirectory;
  conn_state->request.fileName = (char*)fileName;
  conn_state->request.httpVersion = (char*)httpVersion;

  conn_state->data_store.mem_addr = 1;
}

void gsm_test(void) {
  TRACE_CALL_ENTER();

  uint8_t fake_data[1000];
  uint8_t output[64];

  strcpy((char*) fake_data, extended_par_http_headers);
  strcat((char*) fake_data, extended_par);
  strcat((char*) fake_data, crc_str);

  uint32_t crc_hw = 0;
  crc_hw_32((const uint8_t*) extended_par, strlen((char*)extended_par), &crc_hw);
  LOGD("#################### %x %x",
      crc_sw_32((const uint8_t*)extended_par, strlen((char*)extended_par)), crc_hw);

  uart_stub_init();

  test_init_connection(&conn_state);
  uart_stub_fill_fromgsm((BYTE *) fake_data, strlen((char*)fake_data));
  uart_stub_fill_fromgsm(
      (BYTE *) gsm_responses[GSM_RESPONSE_CODE_NO_CARRIER][GSM_NUMERIC_RESPONSE],
      strlen(
          gsm_responses[GSM_RESPONSE_CODE_NO_CARRIER][GSM_NUMERIC_RESPONSE]));

  SST25SectorErase(SST25_CMD_SER04, 0x401B8100);
//	SST25ChipErase()
  //TODO check sector is erased and then check data is written correctly after call to gsm_download_extended_par.

  expected_gsm_request_length = strlen(expected_gsm_request);
  LONGS_EQUAL(1, gsm_download_extended_par(&conn_state));

  t_uart = uart_stub_get_test_data();
//	LOGD("TO GSM");
//	print_buffer_hex(t_uart->to_gsm_buf, t_uart->to_gsm_index);
//	print_buffer(t_uart->to_gsm_buf, t_uart->to_gsm_index);

  STRCMP_EQUAL(expected_gsm_request, t_uart->to_gsm_buf);
  LONGS_EQUAL(expected_gsm_request_length, t_uart->to_gsm_index);

  readStringFromSST25(0x401B8100, output, 24);
  BUFFERS_EQUAL(expected_char, output, 24);

  readStringFromSST25(0x401B8200, output, 20);
  BUFFERS_EQUAL(expected_hex, output, 20);

  readStringFromSST25(RECORDVARIABLES_START, output, 64);
  BUFFERS_EQUAL(expected_par_str, output, 64);

  readStringFromSST25(RECORDVARIABLES_START + 64, output, 64);
  BUFFERS_EQUAL(expected_par_mask, output, 64);

  TRACE_CALL_EXIT();
}

void firmware_download_legacy_test(void) {
  TRACE_CALL_ENTER();

  uint8_t fake_data[1000];
  uint8_t output[1000];
  uint8_t firmware[1000];

  strcpy((char*)firmware, firmware_str);
  strcat((char*)firmware, firmware_checksum_str);

  strcpy((char*) fake_data, extended_par_http_headers);
  strcat((char*) fake_data, (const char*)firmware);

  test_init_connection(&conn_state);
  uart_stub_init();
  uart_stub_fill_fromgsm((BYTE *) fake_data, strlen((char*)fake_data));

  //TODO Removed the No carrier response since the code breaks when it comes with the firmware. Enable this after rewriting the firmware download code!
  //So it is normal that the test waits for waitForString(3) to timeout for now!!
//  uart_stub_fill_fromgsm(
//      (BYTE *) gsm_responses[GSM_RESPONSE_CODE_NO_CARRIER][GSM_NUMERIC_RESPONSE],
//      strlen(
//          gsm_responses[GSM_RESPONSE_CODE_NO_CARRIER][GSM_NUMERIC_RESPONSE]));

  SST25SectorErase(SST25_CMD_SER04, 0x00);

  expected_gsm_request_length = strlen(expected_gsm_request);

  int result = gsm_download_file_http_get_legacy(&conn_state);
  LONGS_EQUAL(1, result);

  t_uart = uart_stub_get_test_data();
//	LOGD("TO GSM");
//	print_buffer_hex(t_uart->to_gsm_buf, t_uart->to_gsm_index);
//	print_buffer(t_uart->to_gsm_buf, t_uart->to_gsm_index);

  STRCMP_EQUAL(expected_gsm_request, t_uart->to_gsm_buf);
  LONGS_EQUAL(expected_gsm_request_length, t_uart->to_gsm_index);

	readStringFromSST25(0x00, output, 1000);
//	print_buffer_hex(output, 1000);
	// output[0] = tells bootloader
	LONGS_EQUAL(0xA5, output[0]);
	BUFFERS_EQUAL(firmware, output + 1, strlen((char*)firmware));

  TRACE_CALL_EXIT();
}

void firmware_download_crc_test(void) {
  TRACE_CALL_ENTER();

  uint8_t fake_data[1000];
  uint8_t output[1000];

  uint8_t firmware[1000];

  strcpy((char*)firmware, firmware_str);
  strcat((char*)firmware, firmware_crc_32_str);

  strcpy((char*) fake_data, extended_par_http_headers);
  strcat((char*) fake_data, (const char*)firmware);

  test_init_connection(&conn_state);
  uart_stub_init();
  uart_stub_fill_fromgsm((BYTE *) fake_data, strlen((char*)fake_data));

  //TODO Removed the No carrier response since the code breaks when it comes with the firmware. Enable this after rewriting the firmware download code!
  //So it is normal that the test waits for waitForString(3) to timeout for now!!
//  uart_stub_fill_fromgsm(
//      (BYTE *) gsm_responses[GSM_RESPONSE_CODE_NO_CARRIER][GSM_NUMERIC_RESPONSE],
//      strlen(
//          gsm_responses[GSM_RESPONSE_CODE_NO_CARRIER][GSM_NUMERIC_RESPONSE]));

  SST25SectorErase(SST25_CMD_SER04, 0x00);

  expected_gsm_request_length = strlen(expected_gsm_request);

  int result = gsm_download_file_http_get(&conn_state);
  LONGS_EQUAL(1, result);

  t_uart = uart_stub_get_test_data();
//	LOGD("TO GSM");
//	print_buffer_hex(t_uart->to_gsm_buf, t_uart->to_gsm_index);
//	print_buffer(t_uart->to_gsm_buf, t_uart->to_gsm_index);

  STRCMP_EQUAL(expected_gsm_request, t_uart->to_gsm_buf);
  LONGS_EQUAL(expected_gsm_request_length, t_uart->to_gsm_index);

	readStringFromSST25(0x00, output, 1000);
//	print_buffer_hex(output, 1000);
	// output[0] = tells bootloader
	LONGS_EQUAL(0xA5, output[0]);
	BUFFERS_EQUAL((const uint8_t*)firmware_str, output + 1, strlen((char*)firmware_str));

  TRACE_CALL_EXIT();
}

void crc_hw_sw_test(void) {
  TRACE_CALL_ENTER();

  uint8_t buf1[] = "123456789";
  uint8_t buf2[] = "abcd";

  uint16_t crc16_expected_1 = 0x29B1;
  uint32_t crc32_expected_1 = 0xCBF43926;
  uint16_t crc16_expected_2 = 0x2CF6;
  uint32_t crc32_expected_2 = 0xED82CD11;

  uint16_t crc16_hw;
  uint32_t crc32_hw;
  uint16_t crc16_sw;
  uint32_t crc32_sw;

  crc_hw_16(buf1, strlen((char*)buf1), &crc16_hw);
  crc_hw_32(buf1, strlen((char*)buf1), &crc32_hw);
  crc16_sw = crc_sw_16(buf1, strlen((char*)buf1));
  crc32_sw = crc_sw_32(buf1, strlen((char*)buf1));

  LONGS_EQUAL(crc16_expected_1, crc16_hw);
  LONGS_EQUAL(crc16_expected_1, crc16_sw);
  LONGS_EQUAL(crc32_expected_1, crc32_hw);
  LONGS_EQUAL(crc32_expected_1, crc32_sw);

  crc_hw_16(buf2, strlen((char*)buf2), &crc16_hw);
  crc_hw_32(buf2, strlen((char*)buf2), &crc32_hw);
  crc16_sw = crc_sw_16(buf2, strlen((char*)buf2));
  crc32_sw = crc_sw_32(buf2, strlen((char*)buf2));

  LONGS_EQUAL(crc16_expected_2, crc16_hw);
  LONGS_EQUAL(crc16_expected_2, crc16_sw);
  LONGS_EQUAL(crc32_expected_2, crc32_hw);
  LONGS_EQUAL(crc32_expected_2, crc32_sw);

  uint32_t address = 0x0000AAAA;
  SST25SectorErase(SST25_CMD_SER04, address);
  writeStringToSST25(address, (BYTE*)firmware_str, strlen((char*)firmware_str));

//  print_flash(address, strlen((char*)firmware_str));
  LONGS_EQUAL(1, check_crc_32_on_flash(address, strlen((char*)firmware_str), firmware_crc_32));
  LONGS_EQUAL(0, check_crc_32_on_flash(address, strlen((char*)firmware_str), 0x0077dff));

  TRACE_CALL_EXIT();
}

extern double table_res_1[];
extern double table_res_2[];
extern double table_res_3[];
extern double table_res_4[];

extern double* res_tables[];
extern int res_table_sizes[];
extern int R_SELECTED_LIMITS[NUMBER_OF_R_TABLES][2];


void hvac_ladder_test(void) {
  TRACE_CALL_ENTER();

  int commissioned = 1;
  int heating_error = 0;

  //Added +1 to some values due to double -> int implicit conversion
  LONGS_EQUAL(R1, lookup_hvac_r_ladder(LOWER_LIMIT(R1) + 50 + 1, commissioned, heating_error));
  LONGS_EQUAL(19312, _pwm_value);

  LONGS_EQUAL(R1, lookup_hvac_r_ladder(UPPER_LIMIT(R1) - 50, commissioned, heating_error));
  LONGS_EQUAL(R2, lookup_hvac_r_ladder(UPPER_LIMIT(R1) + 10, commissioned, heating_error));
  LONGS_EQUAL(6142, _pwm_value);

  LONGS_EQUAL(R2, lookup_hvac_r_ladder(LOWER_LIMIT(R2) + 20, commissioned, heating_error));
  LONGS_EQUAL(R1, lookup_hvac_r_ladder(LOWER_LIMIT(R2) - 1, commissioned, heating_error));
  LONGS_EQUAL(R2, lookup_hvac_r_ladder(UPPER_LIMIT(R2) - 50, commissioned, heating_error));
  LONGS_EQUAL(R3, lookup_hvac_r_ladder((LOWER_LIMIT(R3) + UPPER_LIMIT(R3)) / 2, commissioned, heating_error));
  LONGS_EQUAL(R3, lookup_hvac_r_ladder(UPPER_LIMIT(R3), commissioned, heating_error));
  LONGS_EQUAL(R3, lookup_hvac_r_ladder(LOWER_LIMIT(R3) + 1, commissioned, heating_error));
  LONGS_EQUAL(R4, lookup_hvac_r_ladder((LOWER_LIMIT(R4) + UPPER_LIMIT(R4)) / 2, commissioned, heating_error));
  LONGS_EQUAL(R4, lookup_hvac_r_ladder(LOWER_LIMIT(R4) + 1, commissioned, heating_error));
  LONGS_EQUAL(R_ERROR, lookup_hvac_r_ladder(ABS_UPPER_LIMIT + 10, commissioned, heating_error));
  //Should be initialized to R2 now
  LONGS_EQUAL(R2, lookup_hvac_r_ladder(LOWER_LIMIT(R2) + 1, commissioned, heating_error));
  LONGS_EQUAL(R1, lookup_hvac_r_ladder(LOWER_LIMIT(R2) - 5, commissioned, heating_error));

  TRACE_CALL_EXIT();
}

#include "sensor_data.h"
//#define TYPE_TEMP_SENSOR 0x01
#define HOUSE_1 0x01
#define HOUSE_3 0x03
#define SENSOR_0 0x00
#define SENSOR_2 0x02

//#define TEMPERATURE_1 0x012C //+30 degrees
//#define TEMPERATURE_2 0x000A // +1 degree
//#define TEMPERATURE_3 0xFFF4 //-1.2 degrees the real representation (12 bits): 0x0FF4, easier to test like this
//#define TEMPERATURE_4 0xFEEF //-27.3 degrees
//
////#define TEMPERATURE_BYTES(INDEX) (TEMPERATURE_##INDEX & 0x0f00) >> 8, TEMPERATURE_##INDEX & 0x00ff

/* Format of the data from the temperature sensor
Bit 15  14  13  12  11  10   9    8    7    6    5    4    3     2     1     0
    S   S   S   S   S   2^6  2^5  2^4  2^3  2^2  2^1  2^0  2^-1  2^-2  2^-3  2^-4
Sensor node transmits only the 0-11 bits.
*/

#define TEMPERATURE_1 0x000000A2 //+10.125
#define TEMPERATURE_2 0x00000008 // +0.5
#define TEMPERATURE_3 0xFFFFFE6F //-25.0625 the real representation (12 bits): 0xE6F, easier to test like this
#define TEMPERATURE_4 0x000007D0 //+125 degrees

/*Shift 12 bits to left to get 16.16 precision we use for floating points*/
#define TEMPERATURE_1_HIGH_PRECISION 0x000A2000
#define TEMPERATURE_2_HIGH_PRECISION 0x00008000
#define TEMPERATURE_3_HIGH_PRECISION 0xFFE6F000
#define TEMPERATURE_4_HIGH_PRECISION 0x007D0000

#define HUMIDITY 0x12
#define PAD_BYTE 0x00
#define BITS PAD_BYTE

void sensor_test_helper(uint8_t type, uint8_t house_code, uint8_t sensor_code,
                        int32_t temperature, int32_t converted_temperature, int read_error_result) {
  sensor_data_t sensor_data;
  uint8_t buf_sensor_data_1[] = {type, PAD_BYTE, house_code << 4 | sensor_code, BITS,
                                 ((temperature & 0x00000f00) >> 8), temperature & 0x000000ff, HUMIDITY, PAD_BYTE, EOL_ISMPIC};

  LONGS_EQUAL(SENSOR_DATA_ROW_LENGTH, sizeof(buf_sensor_data_1));

  //  LONGS_EQUAL(1, sensor_data_deserialize(buf_sensor_data_1, &sensor_data));
  sensor_data_deserialize(buf_sensor_data_1, &sensor_data);

  LONGS_EQUAL(type, sensor_data.type);
  LONGS_EQUAL(house_code, sensor_data.house_code);
  LONGS_EQUAL(sensor_code, sensor_data.sensor_code);
  LONGS_EQUAL(temperature, sensor_data.temperature);
  LONGS_EQUAL(HUMIDITY, sensor_data.humidity);

  LONGS_EQUAL(read_error_result, sensor_data_read(buf_sensor_data_1, &sensor_data));
  LONGS_EQUAL(converted_temperature, sensor_data.temperature);
}

void sensor_data_test(void) {
  TRACE_CALL_ENTER();

  sensor_test_helper(TYPE_TEMP_SENSOR, HOUSE_1, SENSOR_2, TEMPERATURE_1, TEMPERATURE_1_HIGH_PRECISION, 0);
  sensor_test_helper(TYPE_TEMP_SENSOR, HOUSE_3, SENSOR_0, TEMPERATURE_2, TEMPERATURE_2_HIGH_PRECISION, 0);
  sensor_test_helper(TYPE_TEMP_SENSOR, HOUSE_1, SENSOR_0, TEMPERATURE_3, TEMPERATURE_3_HIGH_PRECISION, 0);
  /*Temperature is lower than -45 so we expect an error*/
  sensor_test_helper(TYPE_TEMP_SENSOR, HOUSE_3, SENSOR_2, TEMPERATURE_4, TEMPERATURE_4_HIGH_PRECISION, 1);

  TRACE_CALL_EXIT();
}


int main() {
  //Initializations
  atc_platform_init_port_pins();
  atc_platform_init_com_ports();
  tracer_init();

  pic32_print_hardware_details();

//  crc_hw_sw_test();
//  gsm_test();
//  firmware_download_legacy_test();
//  firmware_download_crc_test();
//  hvac_ladder_test();

  sensor_data_test();

  return 0;
}
