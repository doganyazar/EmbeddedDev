/*
 * crc_hw.h
 *
 *  Created on: Aug 23, 2013
 *      Author: dogan
 */

#ifndef CRC_HW_H_
#define CRC_HW_H_

//CRC-CCITT (0xffff)
int crc_hw_16(const uint8_t* data, int size, uint16_t* out);

//CRC-32
int crc_hw_32(const uint8_t* data, int size, uint32_t* out);


int check_crc_32_on_flash(uint32_t address, int size, uint32_t crc_compare);
#endif /* CRC_HW_H_ */
