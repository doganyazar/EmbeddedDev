#include <stdio.h>
#include <stdint.h> //uint32_t etc
#include "pic32_test_defs.h"
#include "delay.h"
#include "HardwareProfile.h"
#include "memorymap.h"

#include "main.h" //TODO remove!
#include "uarts.h"
#include "SST25.h"
#include "common_test_utils.h"

#define PRINT_MAP(A, B) \
    LOG_TEST(#A ": %x " #B ": %x ", A, B);

int pic32_test_assert_longs_equal(long expected, long actual, const char* fileName, int lineNumber)
{
	int result = 0;
	if (expected != actual) {
	    LOG_TEST("### FAIL %ld vs %ld in %s:%d", expected, actual, fileName, lineNumber);
	} else {
	    LOG_TEST("OK %ld in %s:%d", actual, fileName, lineNumber);
		result = 1;
	}

	return result;
}

int pic32_test_assert_cstr_equal(const char* expected, const char* actual, const char* fileName, int lineNumber) {
    int result = 0;
    if (strcmp(expected, actual) != 0) {
        LOG_TEST("### FAIL %s vs %s in %s:%d", expected, actual, fileName, lineNumber);
    } else {
        LOG_TEST("OK %s in %s:%d", actual, fileName, lineNumber); \
        result = 1;
    }

    return result;
}

int pic32_test_assert_buffers_equal(const uint8_t* expected, const uint8_t* actual, int size, const char* fileName, int lineNumber) {
    int result = 1;
    int i = 0;
    for (i = 0; i < size; i++) {
        if (expected[i] != actual[i]) {
            result = 0;
            break;
        }
    }

    if (!result) {
        LOG_TEST("### FAIL %s:%d", fileName, lineNumber);
    } else {
        LOG_TEST("OK %s:%d", fileName, lineNumber); \
    }

    return result;
}

void pic32_print_hardware_details(void){
    int size_short = sizeof(short);
    int size_int = sizeof(int);
    int size_long = sizeof(long);
    uint32_t i=0x01234567;

    // 0 for big endian, 1 for little endian.
    int is_little_endian = (*((uint8_t*)(&i))) == 0x67;

    LOG_TEST("sizes: short %d, int %d, long %d", size_short, size_int, size_long);
    LOG_TEST("is little endian: %d", is_little_endian);

    //Memory mapping
    LOG_TEST("RAM Size: %lx", (unsigned long)0x20000);
    LOG_TEST("FLASH Size: %lx", (unsigned long)0x82FF0);

    PRINT_MAP(PIC32RAM_START, PIC32RAM_LENGTH);
    PRINT_MAP(PIC32FLASH_START, PIC32FLASH_LENGTH);
    PRINT_MAP(ICPCODE_START, ICPCODE_LENGTH);
    PRINT_MAP(NODEVARIABLES_START, NODEVARIABLES_LENGTH);
    PRINT_MAP(RECORDVARIABLES_START, RECORDVARIABLES_LENGTH);
    PRINT_MAP(COMFORTCALENDAR_START, COMFORTCALENDAR_LENGTH);
    PRINT_MAP(ENABLECALENDAR_START, ENABLECALENDAR_LENGTH);
    PRINT_MAP(EVENTLOG_START, EVENTLOG_LENGTH);
    PRINT_MAP(DATALOG_START, DATALOG_LENGTH);

}

void print_flash_hex(uint32_t address, int size) {
  char buffer[1000];
  readStringFromSST25(address, (BYTE*)buffer, size);

  print_buffer_hex(buffer, size);
}

void print_flash(uint32_t address, int size) {
  char buffer[1000];
  readStringFromSST25(address, (BYTE*)buffer, size);

  print_buffer(buffer, size);
}

void pic32_test_swapping_led(void){
	while(1){
		LED_SERVICE_INV;
		DelayMs(1000);
	}
}
