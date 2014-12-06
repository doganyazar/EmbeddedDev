#include <stdlib.h>
#include <stdint.h>
#include <plib.h> //DMA_ macros
#include "utils.h"
#include "tracer.h"

#include "SST25.h" //to be removed later after check_crc_32_on_flash is removed.

const uint32_t crc16_polynomial = 0x1021;
const uint16_t crc16_converted_seed = 0x84cf; //instead of 0xffff
const int crc16_polynomial_len = 16;

const uint32_t crc32_polynomial = 0x04c11db7;
const uint32_t crc32_seed = 0xffffffff;
const uint32_t crc32_converted_seed = 0x46af6449; //instead of 0xffffffff
const int crc32_polynomial_len = 32;

uint16_t crc_hw_16_orig(const uint8_t* data, int len, uint16_t poly,
    uint16_t seed, uint16_t* out) {
  unsigned int crc;
  int chn = 2;
  DmaTxferRes res;

  DmaSfmConfigure(DMA_CHKSUM_CRC, DMA_BITO_MSb, DMA_REORDER_NOT);
  DmaCrcConfigure(poly, crc16_polynomial_len, seed);

  res = DmaChnMemCrc(&crc, data, len, chn + 1, DMA_CHN_PRI2); // we could use for this example a different DMA channel
  if (res != DMA_TXFER_OK) {
    return 0; // failed
  }

  *out = (uint16_t) crc;

  return 1;
}

//CRC-CCITT (0xffff)
int crc_hw_16(const uint8_t* data, int size, uint16_t* out) {
  int success = 0;
  uint16_t crc = 0;
  uint16_t padding = 0;

  //Step 1 - call the original crc hw function with the converted polynomial
  if (!crc_hw_16_orig(data, size, crc16_polynomial, crc16_converted_seed,
      &crc)) {
    return success;
  }

  //Step 2 - call with extra padding of 0s (16 bit 0s). Seed is crc value obtained from the previous step.
  if (!crc_hw_16_orig((uint8_t*) &padding, sizeof(uint16_t), crc16_polynomial,
      crc, &crc)) {
    return success;
  }

  success = 1;
  *out = crc;

  return success;
}

int crc_hw_32_orig(const uint8_t* data, int len, uint32_t poly, uint32_t seed,
    uint32_t* out) {
  unsigned int crc;
  int chn = 2;
  DmaTxferRes res;

  //Somehow setting Bit0 to be LSB is necessary for the crc 32
  DmaSfmConfigure(DMA_CHKSUM_CRC, DMA_BITO_LSb, DMA_REORDER_NOT);

  DmaCrcConfigure(poly, crc32_polynomial_len, seed);

  res = DmaChnMemCrc(&crc, data, len, chn + 1, DMA_CHN_PRI2); // we could use for this example a different DMA channel

  if (res != DMA_TXFER_OK) {
    return 0;      // failed
  }

  *out = (uint32_t) crc;
  return 1;
}

int crc_hw_32_add(const uint8_t* data, int len, uint32_t seed, uint32_t* out) {
  return crc_hw_32_orig(data, len, crc32_polynomial, seed, out);
}

int crc_hw_32_final(uint32_t crc, uint32_t* out) {

  int success = 0;
  uint32_t padding = 0;
  //call with extra padding of 0s (32 bit 0s). Seed is crc value obtained from the previous step.
  if (!crc_hw_32_add(
      (uint8_t*) &padding,
      sizeof(uint32_t),
      crc,
      &crc)) {
    return success;
  }

  success = 1;
  //Mirror the crc
  crc = mirror_uint32(crc);
  //Xor with the seed again
  crc ^= crc32_seed;

  *out = crc;
  return success;
}

//CRC-32
int crc_hw_32(const uint8_t* data, int size, uint32_t* out) {
  int success = 0;
  uint32_t crc = 0;

  //Step 1 - call the original crc hw function with the converted polynomial
  if (!crc_hw_32_add(
      data,
      size,
      crc32_converted_seed,
      &crc)) {
    return success;
  }

  //Step 2 - call with extra padding of 0s (32 bit 0s). Seed is crc value obtained from the previous step.
  //Step 3 - Mirror the crc
  //Step 4 - Xor with the seed again
  success = crc_hw_32_final(crc, out);

  return success;
}


//TODO DY this function should be moved (with a proper prefix) to reading/writing to flash related source file.
int check_crc_32_on_flash(uint32_t address, int total_size, uint32_t crc_compare) {
  //TODO for now it uses a stack buffer of fixed size to read data in chunks
  //since total_size maybe quite big. Would be better to introduce some global general purpose buffers.
  uint8_t temp_buf[1024];
  int buf_size = 1024;

  int acc_len = 0;
  int success = 0;
  int read_len = buf_size;
  uint32_t crc_acc = crc32_converted_seed;

  for ( ;acc_len < total_size; acc_len += buf_size) {

    if (acc_len + read_len > total_size) {
      read_len = total_size - acc_len;
    }

    readStringFromSST25(address + acc_len, temp_buf, read_len);

    if (!crc_hw_32_add(
        temp_buf,
        read_len,
        crc_acc,
        &crc_acc)) {
      return success;
    }
  }

  success = crc_hw_32_final(crc_acc, &crc_acc);

  if (success) {
    success = (crc_compare == crc_acc);
  }

  return success;
}
