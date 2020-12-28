#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "/Users/ashu/eclipse-workspace/libc-atmega328/hal_definitions.h"
#include "/Users/ashu/eclipse-workspace/libc-atmega328/hal_uart.h"

void echo_function(void)
{
	hal_error err;
	Byte byt;

	while(1)
	{
		err=hal_uart_get_byte(&byt);

		if( err == HAL_NO_ERROR )
		{
			hal_uart_write(byt);

			if( byt == 't' )
			{
				hal_uart_println("terminating hal uart driver");
				return;
			}
		}
	}

}

int main()
{
	/* configure the UART settings here */
	hal_uart_config conf={
			._baud=9600,
			._parity=NO_PARITY,
			._stopBits=1,
			._buffSize=50,
	};


	hal_uart_driver_install(conf);

	echo_function();

	hal_uart_driver_uninstall();

	hal_uart_flush_buffer();

	_delay_ms( 5000 );

	hal_uart_driver_install(conf);

	hal_uart_println("reinstalled driver");

	echo_function();

	return 0;
}
