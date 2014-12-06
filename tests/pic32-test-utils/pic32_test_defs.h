/*
 * common_test_defs.h
 *
 *  Created on: Aug 7, 2013
 *      Author: dogan
 */

#ifndef PIC32_TEST_DEFS_H_
#define PIC32_TEST_DEFS_H_

#include <stdint.h>
#include "tracer.h"

//#ifndef true
//	#define true 1
//#endif
//
//#ifndef false
//	#define false 0
//#endif

/*This macro is always enabled since test code should never be in production code anyway*/
#define LOG_TEST(...) LOGGER(__VA_ARGS__)


//TODO should move these to some common place for all kind of tests.
#define BYTES_EQUAL(expected, actual)\
    LONGS_EQUAL(((expected) & 0xff),((actual) & 0xff))

//Check two long integers for equality
#define LONGS_EQUAL(expected,actual)\
do { \
    pic32_test_assert_longs_equal(expected, actual, __FILE__, __LINE__);\
} while(0)

#define STRCMP_EQUAL(expected, actual) \
do { \
    pic32_test_assert_cstr_equal(expected, actual, __FILE__, __LINE__);\
} while(0)

#define BUFFERS_EQUAL(expected, actual, size) \
do { \
    pic32_test_assert_buffers_equal(expected, actual, size, __FILE__, __LINE__);\
} while(0)

int pic32_test_assert_longs_equal(long expected, long actual, const char* fileName, int lineNumber);
int pic32_test_assert_buffers_equal(const uint8_t* expected, const uint8_t* actual, int size, const char* fileName, int lineNumber);
int pic32_test_assert_cstr_equal(const char* expected, const char* actual, const char* fileName, int lineNumber);


void pic32_test_swapping_led(void);
void pic32_print_hardware_details(void);


void print_flash_hex(uint32_t address, int size);
void print_flash(uint32_t address, int size);

#endif /* PIC32_TEST_DEFS_H_ */
