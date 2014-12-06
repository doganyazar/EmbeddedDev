#include <stdio.h>
#include <string.h>
#include <stdlib.h> /*strtol*/
#include <ctype.h> /*Needed for isspace*/
#include "tracer.h"
#include "utils.h"

inline uint8_t* __attribute__((always_inline)) multi_strchr(const uint8_t* str, uint8_t* keys){
    uint8_t* result = NULL;
    int i = strcspn ((const char*)str, (const char*)keys);
    if (str[i]) {
        result = (uint8_t*)str + i;
    }
    return result;
}

const uint8_t* read_hex(const uint8_t* buf, uint32_t* out_value) {
    uint8_t* result = NULL;

    *out_value = strtol((const char*)buf, (char **)&result, 16);
    if (buf == result) {
        result = NULL;
    }

    return result;
}

const uint8_t* skip_whitespace(const uint8_t* buf) {
    for (; buf && *buf; buf++) {
        if (isspace(*buf)) {
            continue;
        } else {
            break;
        }
    }

    return buf;
}

const uint8_t* skip_blank_lines(const uint8_t* buf) {
    for (; buf && *buf; buf++) {
        if (*buf == '\n' || *buf == '\r') {
            continue;
        } else {
            break;
        }
    }

    return buf;
}

//mirror bits of each byte, i.e. bit n <-> bit 7-n
//can be done in place, i.e. dest_buf = src_buf
void mirror_bytes(uint8_t* dest_buf, uint8_t* src_buf, int size) {
    int     ix, jx;
    uint8_t   src, dst, lsb;

    for (ix = 0; ix < size; ix++) {
        src = *src_buf++;
        dst = 0;
        for (jx = 0; jx < 8; jx++) {
            lsb = src&1;
            src >>= 1;
            dst <<= 1;
            dst |= lsb;
        }
        *dest_buf++ = dst;
    }
}

//swap the endianness of the 32 bit integer
uint32_t swap_uint32(uint32_t input) {
    uint32_t swapped =
            ((input >> 24) & 0xff) | ((input << 8) & 0xff0000)
            | ((input >> 8) & 0xff00) | ((input << 24) & 0xff000000);

    return swapped;
}

//Mirror bits of 32 bit number, i.e. bit n <-> bit 31-n
uint32_t mirror_uint32(uint32_t input) {
    uint32_t mirrored = 0;

    mirror_bytes((uint8_t*)&mirrored, (uint8_t*)&input, sizeof(uint32_t));
    mirrored = swap_uint32(mirrored);

    return mirrored;
}
