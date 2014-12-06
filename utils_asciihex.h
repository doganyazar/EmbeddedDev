#include <stdint.h>
#include "GenericTypeDefs.h"

//extern int __attribute__((always_inline)) asciihex2binChar( char* c ); //REMOVED
extern int          __attribute__((always_inline)) asciihex2binChar( BYTE* c ); //ADDED
extern UINT16_VAL   binChar2AsciiHex( UINT8_VAL binChar );
extern int          hex2str( int value, char* string );
uint8_t asciihexbytes_to_byte(uint8_t high, uint8_t low);
int asciihexbytes_to_binary_buf(uint8_t* buf, int buf_len);
