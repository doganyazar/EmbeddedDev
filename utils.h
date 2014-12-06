/*
 * utils.h
 *
 *  Created on: Aug 15, 2013
 *      Author: dogan
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

inline uint8_t* __attribute__((always_inline)) multi_strchr(const uint8_t* str, uint8_t* keys);

//void util_parse_line(const char* line);
//int util_parse_extended(const char* data);

const uint8_t* read_hex(const uint8_t* buf, uint32_t* out_value);
const uint8_t* skip_whitespace(const uint8_t* buf);
const uint8_t* skip_blank_lines(const uint8_t* buf);

void mirror_bytes(uint8_t* dest_buf, uint8_t* src_buf, int size);
uint32_t swap_uint32(uint32_t input);
uint32_t mirror_uint32(uint32_t input);

#endif /* UTILS_H_ */
