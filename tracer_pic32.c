#include "tracer.h"
#include "uarts.h"
#include "main.h" //TODO Needed for UART_DEBUG. Remove later!
#include <p32xxxx.h>
#include <stdio.h>

#ifndef UNIT_TESTING //TODO Somehow does not work with unit testing. Check out!
/* Needed to redirect printf to our debug serial port. Default is directed to UART2
 * Details: http://microchip.wikidot.com/xc32:how-to-redirect-stdout-for-use-with-printf
 */
void _mon_putc(char c)
{
	// printf() passes each character to write to _mon_putc(),
	//which in turn passes each character to a custom output function
    writeByteToUxRTx(UART_DEBUG, (BYTE)c);
}
#endif /*UNIT_TESTING*/

void tracer_init(void){
	setbuf(stdout, NULL);
}
