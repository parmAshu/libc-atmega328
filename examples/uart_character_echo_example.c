#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "/Users/ashu/eclipse-workspace/libc-atmega328/hal_definitions.h"
#include "/Users/ashu/eclipse-workspace/libc-atmega328/hal_uart.h"

int main()
{
	/* configure the UART settings here */
	hal_uart_config conf={
			._baud=9600,
			._parity=NO_PARITY,
			._stopBits=1,
			._buffSize=50,
	};

	Byte byt;
	hal_error err;

	err = hal_uart_driver_install(conf);

	while(1)
	{
		err=hal_uart_get_byte(&byt);

		if( err == HAL_NO_ERROR )
		{
			hal_uart_write(byt);
		}
	}
	return 0;
}
