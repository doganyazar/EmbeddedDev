#include "GenericTypeDefs.h"
#include <string.h>
//#include "main.h"
#include "utils_asciihex.h"

/*****************************************************************************
* Name:         asciihex2bin
*
* Input:        ASCII character to convert
*               char pointer
*
* Output:       0 is false code
*               else is ok
*
* Description:  Converts an ASCII-HEX value to its corresponding hex-value
*
* Remarks:      The result is put back in the input argument
*               Error handlign NOT implemented
*
* Use:          asciihex2bin( &c );
*****************************************************************************/
//int __attribute__((always_inline)) asciihex2binChar( char* c ) //REMOVED
int __attribute__((always_inline)) asciihex2binChar( BYTE* c ) //ADDED
{	
	*c = ((*c < 'A') ? *c - '0' : ((*c < 'a') ? *c - 'A' + 0xA : *c - 'a' + 0xA ));
	*c &= 0x0F;
	
 	return 1;
}

uint8_t asciihexbytes_to_byte(uint8_t high, uint8_t low){
    asciihex2binChar(&high);
    asciihex2binChar(&low);

    return (high << 4) + low;
}

//Assumes even buffer size and updates in place!!!
int asciihexbytes_to_binary_buf(uint8_t* buf, int buf_len) {
  int result = 1;
  int i = 0;
  int j = 0;

  for (i = 0, j = 0; i < buf_len - 1; i += 2, j++) {
    buf[j] = asciihexbytes_to_byte(buf[i], buf[i + 1]);
  }

  return result;
}

UINT16_VAL binChar2AsciiHex( UINT8_VAL binChar )
{
	UINT16_VAL	asciiHex	__attribute__ ((aligned (8)));
	char		c			__attribute__ ((aligned (8)));
	
	c = binChar.nibble.HN;
	asciiHex.byte.HB = (c < 0xA) ? c + '0' : c + 'A' - 0xA;
	c = binChar.nibble.LN;
	asciiHex.byte.LB = (c < 0xA) ? c + '0' : c + 'A' - 0xA; 

//        writeByteToUxRTx( UART_DEBUG, asciiHex.byte.HB );
//        writeByteToUxRTx( UART_DEBUG, asciiHex.byte.LB );
	return asciiHex;
}

/*****************************************************************************
* Name:         hex2str
*
* Input:        Integral value to convert.
*               Char pointer.
*
* Output:       0 is false code
*               else is number of characters in 'string'.
*
* Description:  Converts a integer value to a string of its corresponding
*               hex-values.
*
* Remarks:      The input string have to be att least 8 characters long.
*
* Use:          characters_in_resulting_string =
*                                    hex2str( integer_value, resulting_string );
*****************************************************************************/
//int __attribute__((always_inline)) asciihex2binChar( char* c ) //REMOVED
int hex2str( int value, char* string )
{
	UINT16_VAL	asciiHex	__attribute__ ((aligned (8)));
	char		c			__attribute__ ((aligned (8)));
	int 		i, j;
	UINT32_VAL	word;

        // Check if input string is to short.
//        if( sizeof( string ) < 8 )
//            return 0;
        
        // Clear string.
        memset( string, 0, 8 );

        word = (UINT32_VAL)((UINT32)value);
	for( j = 3, i = 0; j >= 0; j-- )
	{
            // Extract byte.
            c = word.v[j];

            //Convert to byte to ASCII-HEX.
            asciiHex = binChar2AsciiHex( (UINT8_VAL)((UINT8)c) );

            //Insert ASCII-HEX into string.
            string[i++] = asciiHex.byte.HB;
            string[i++] = asciiHex.byte.LB;
	}

        return i;
}
