#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/* including the required HAL headers*/
#include "hal_definitions.h"
#include "hal_uart.h"

/* The baud rate pre-scaler */
#define BAUD_PRESCALER (((F_CPU/(config._baud*16UL)))-1)

/* UART receive ring buffer variables */
static Byte * rxBuffer = NULL;
static uint8_t rxBuffSize;
static uint8_t inPos;
static uint8_t outPos;
static uint8_t filled;
static uint32_t num_overflows;

static uint32_t ind=0;

/* hal driver installed state variable */
char hal_uart_driver_installed=0;

/* UART receive interrupt service routine */
ISR(USART_RX_vect)
{
	if(hal_uart_driver_installed)
	{
		if( filled < rxBuffSize )
		{
			rxBuffer[ inPos ++ ] = UDR0;
			filled++;
			if(inPos == rxBuffSize) inPos=0;
		}
		else
			num_overflows++;
	}
}

/* API for initialization of UART port
 *
 * PARAMETERS -
 * 1. hal_uart_config config : a structure that store the configuration details for the UART port. The configuration
 * parameters are copied from the structure during the initialization process. The structure is not needed afterwards
 * and may be deleted to save memory.
 *
 * RETURNS : hal_error type -
 * 1. HAL_INV_PARAMS : if any of the configuration parameter is not valid.
 * 2. HAL_FAIL : if there is any error during the UART port initialization.
 * 3. HAL_NO_ERROR : if the driver is installed successfully.
 *
 * NOTE -
 * The driver creates a ring buffer for its operation; the size of ring buffer must be passed as a member of the
 * 'config' structure. If the ring buffer could not be created then, driver is not installed and error code is returned.
 * Since, the ring buffer is dynamically created, the user program can modify its size its the situations demand so.
 * For this the user program will have to uninstall the driver and reinstall it with the new buffer size.
 * */
hal_error hal_uart_driver_install(hal_uart_config config)
{
	hal_uart_driver_installed=rxBuffSize=inPos=outPos=filled=num_overflows=0;

	// if the passed buffer size is larger than the maximum allowed value then, return with error code
	if( config._buffSize > MAX_HAL_UART_BUFF_SIZE)
	{
		/* !! when the watch timer is installed, reset it over here */
		return HAL_INV_PARAMS;
	}

	// creating and initializing the ring buffer
	rxBuffer = (Byte *)calloc( (size_t)config._buffSize, sizeof(Byte));

	// if the ring buffer could not be created then, return with error code
	if( rxBuffer == NULL )
	{
		/* !! when the watch timer is installed, reset it over here */
		return HAL_FAIL;
	}

	// 0X86h is the default value of UCSRC register
	Byte ucsrc = 0X06;

	// configure the parity mode
	switch(config._parity)
	{
		case NO_PARITY:
			ucsrc &= (~( (1<<UPM01) | (1<<UPM00) ) );
			break;

		case EVEN_PARITY :
			ucsrc |= (1<<UPM01);
			ucsrc &= (~(1<<UPM00));
			break;

		case ODD_PARITY:
			ucsrc |= ( (1<<UPM01) | (1<<UPM00) );
			break;

		// If the parity is not a valid one then return with error code
		default:
			free(rxBuffer);
			rxBuffer=NULL;

			/* !! when the watch timer is installed, reset it over here */
			return HAL_INV_PARAMS;
	}


	// configure the number of stop bits
	if(config._stopBits==2)
		ucsrc |= (1<<USBS0);
	else
		ucsrc &= (~(1<<USBS0));

	// configure the word length to 8 bits (fixed)
	ucsrc |= (1<<UCSZ00) | (1<<UCSZ01);

	// initialize the UART transmitter, receiver and receive interrupt
	UCSR0B |= (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);

	UCSR0C = ucsrc;

	// configure the baud rate
	UBRR0L = BAUD_PRESCALER;
	UBRR0H = (BAUD_PRESCALER >> 8 );

	rxBuffSize=config._buffSize;
	hal_uart_driver_installed=1;

	// enable the global interrupt
	sei();

	/* !! when the watch timer is installed, reset it over here */
	return HAL_NO_ERROR;
}

/* API to de-initialize the UART port
 *
 * PARAMETERS -
 * none
 *
 * RETURNS : hal_error type -
 * 1. HAL_NO_ERROR : when the driver is uninstalled.
 *
 * NOTE -
 * If the driver was previously installed then, this API will disable the UART port and delete the ring buffer
 * created during the installation.
 *  */
hal_error hal_uart_driver_uninstall(void)
{
	if( hal_uart_driver_installed )
	{
		// disable the UART port
		UCSR0B &= ( ~ ( (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0) ) );

		// delete the ring buffer from the memory
		free(rxBuffer);

		rxBuffer=NULL;

		// reset the buffer state variables
		hal_uart_driver_installed=rxBuffSize=inPos=outPos=filled=num_overflows=0;

		rxBuffer=NULL;
	}

	/* !! when the watch timer is installed, reset it over here */
	return HAL_NO_ERROR;
}


/* API to send a byte over the UART port; this API sends the byte in synchronous fashion and blocks the main task
 * until the byte is sent over the UART port.
 *
 * PARAMETERS -
 * 1. Byte ch : the byte that has to be sent over the UART port
 *
 * RETURNS : hal_error type -
 * 1. HAL_FAIL : if the driver is not installed.
 * 2. HAL_NO_ERROR : if the byte is sent successfully.
 * */
hal_error hal_uart_write(Byte ch)
{
	// if the driver is not installed, then return with error code
	if( ! hal_uart_driver_installed )
	{
		/* !! when the watch timer is installed, reset it over here */
		return HAL_FAIL;
	}

	while( (UCSR0A & (1<<UDRE0)) == 0 );
	UDR0 = ch;

	/* !! when the watch timer is installed, reset it over here */
	return HAL_NO_ERROR;
}


/* API to send multiple bytes over the UART port;this API is a helper API and uses the hal_uart_write() API to
 * perform its task.
 *
 * PARAMETERS -
 * 1. Byte * str : pointer to a 'Byte' type array.
 * 2. uint32_t len : number of bytes from the 'Byte' type array to be sent over the UART port.
 *
 * RETURNS : hal_error type -
 * 1. HAL_NO_ERROR : if all the bytes are sent successfully.
 * 2. HAL_FAIL : if the driver is not installed.
 * */
hal_error hal_uart_write_bytes(Byte * str, uint32_t len)
{
	hal_error err=HAL_FAIL;

	for( ;len>0 ;len--)
	{
		err=hal_uart_write( (*str) );

		if( err != HAL_NO_ERROR)
			break;

		str = str + 1;
	}

	/* !! when the watch timer is installed, reset it over here */
	return err;
}

/* API to send multiple bytes terminated by newline character, over the UART port; this API is a helper API and
 * uses the hal_uart_write_bytes() and hal_uart_write() APIs to perform its task
 *
 * PARAMETERS -
 * 1. Byte * str : pointer to 'Byte' type array.
 * 2. uint32_t len : number of bytes from the 'Byte' type array that are to be sent over the UART port.
 *
 * RETURNS : hal_error type -
 * 1. HAL_FAIL : if the driver is not installed.
 * 2. HAL_NO_ERROR : is the bytes are sent successfully.
 * */
hal_error hal_uart_write_bytes_nl(Byte * str, uint32_t len)
{
	hal_error err;

	err=hal_uart_write_bytes( str, len );

	// send the newline character over the UART port
	if( err == HAL_NO_ERROR )
		err=hal_uart_write( '\n' );

	/* !! when the watch timer is installed, reset it over here */
	return err;
}


/* API to send a null terminated string over the UART port; this is a helper API that uses hal_uart_write() API to
 * perform its task.
 *
 * PARAMETERS -
 * 1. Byte * str : pointer to 'Byte' type array containing null terminated string.
 *
 * RETURNS : hal_error type -
 * 1. HAL_FAIL : if the driver is not installed.
 * 2. HAL_NO_ERROR : is the bytes are sent successfully.
 * */
hal_error hal_uart_print(Byte * str)
{
	hal_error err=HAL_FAIL;

	while( (*str) != '\0' )
	{
		err=hal_uart_write( (*str) );

		if( err != HAL_NO_ERROR)
			break;

		str = str + 1;
	}

	/* !! when the watch timer is installed, reset it over here */
	return err;
}

/* function to send a string over the UART port; this API is a helper API and uses the hal_uart_print() and hal_uart_write()
 * APIs to perform its task.
 *
 * PARAMETERS -
 * 1. Byte * str : pointer to 'Byte' type array containing null terminated string.
 *
 * RETURNS : hal_error type -
 * 1. HAL_FAIL : if the driver is not installed.
 * 2. HAL_NO_ERROR : is the bytes are sent successfully.
 */
hal_error hal_uart_println(Byte * str)
{
	hal_error err=HAL_FAIL;

	err=hal_uart_print(str);

	if( err == HAL_NO_ERROR )
		err=hal_uart_write('\n');

	/* !! when the watch timer is installed, reset it over here */
	return err;
}


/* API to read a byte from the ring buffer into the passed 'Byte' type variable.
 *
 * PARAMETERS -
 * 1. Byte * byt : pointer to 'Byte' type variable where the data byte is to be stored.
 *
 * RETURNS : hal_error type -
 * 1. HAL_NO_ERROR : if the data byte was successfully stored in the specified memory location.
 * 2. HAL_NOT_AVAIL : if no data is available at current instant.
 * 3. HAL_FAIL : if the driver is not installed.
 *
 * NOTE -
 * As of now the program does not perform safety check before attempting to store the data value in the passed
 * memory location. The user program must ensure that the passed pointer points to a valid memory location. If
 * this condition is not satisfied then, the entire program will terminate !
 * */
hal_error hal_uart_get_byte(Byte * byt)
{
	if( hal_uart_driver_installed )
	{
		if( filled )
		{
			(*byt)=rxBuffer[outPos++];
			filled--;
			if(outPos==rxBuffSize) outPos=0;

			/* !! when the watch timer is installed, reset it over here */
			return HAL_NO_ERROR;
		}
		else
		{
			/* !! when the watch timer is installed, reset it over here */
			return HAL_NOT_AVAIL;
		}
	}
	else
	{
		/* !! when the watch timer is installed, reset it over here */
		return HAL_FAIL;
	}
}

/* API to read a specified number of bytes from the UART ring buffer into a passed 'Byte' type array
 *
 * PARAMETERS -
 * 1. Byte * byt : pointer to 'Byte' type array where the data bytes are to be stored.
 * 2. uint8_t length : the number of bytes that needed to be read from the UART ring buffer.
 *
 * RETURNS : hal_error type -
 * 1. HAL_NO_ERROR : if the data bytes were successfully stored in the specified memory location.
 * 2. HAL_NOT_AVAIL : if no data is available at current instant.
 * 3. HAL_FAIL : if the driver is not installed.
 *
 * NOTE -
 * This API does not perform any bound checking before storing the data bytes in the destination array. The user
 * program must take care that the destination array is larger than or equal to the 'length' parameter. If this
 * condition is not satisfied then, the entire program will terminate !
 * */
hal_error hal_uart_get_bytes_num( Byte * str, uint8_t length )
{
	if( hal_uart_driver_installed )
	{
		if(filled >= length )
		{
			Byte ch;
			Byte * _ch = &ch;

			//read the length number of bytes from the UART port and store them in the passed string
			for(;length>0;length--)
			{
				hal_uart_get_byte(_ch);
				(*str)=ch;
				str = str + 1;
			}

			/* !! when the watch timer is installed, reset it over here */
			return HAL_NO_ERROR;
		}
		else
		{
			/* !! when the watch timer is installed, reset it over here */
			return HAL_NOT_AVAIL;
		}
	}
	else
	{
		/* !! when the watch timer is installed, reset it over here */
		return HAL_FAIL;
	}
}


/* API to read bytes until a specified byte, from the UART ring buffer into a passed 'Byte' type array
 *
 * PARAMETERS -
 * 1. Byte * byt : pointer to 'Byte' type array where the data bytes are to be stored.
 * 2. uint32_t * len : pointer to a location where the size of passed buffer MUST be stored; the API also uses
 * this location to store the number of bytes copied into the buffer once the operation is complete.
 * 3. Byte delimiter : the delimiter byte.
 *
 * RETURNS : hal_error type -
 * 1. HAL_NO_ERROR : if the data bytes were successfully stored in the specified memory location.
 * 2. HAL_NOT_AVAIL : if no data is available at current instant.
 * 3. HAL_FAIL : if the driver is not installed.
 *
 * NOTE -
 * 1. This API does not perform any bound checking before storing the data bytes in the destination array. The user
 * program must take care that the destination array is larger than or equal to the 'length' parameter.
 * 2. Also the user program must ensure that the 'len' pointer also points to a valid memory location.
 * If these conditions are not satisfied then, the entire program will terminate !
 * */
hal_error hal_uart_get_bytes_until( Byte * str, uint32_t * len, Byte delimiter)
{
	if( hal_uart_driver_installed )
	{
		if(filled)
		{
			if( ind < (*len) )
			{
				Byte ch;
				hal_uart_get_byte( &ch );

				if( ch == delimiter )
				{
					(*len)=ind;
					ind=0;

					/* !! when the watch timer is installed, reset it over here */
					return HAL_NO_ERROR;
				}
				else
				{
					str[ ind++ ] = ch;

					/* !! when the watch timer is installed, reset it over here */
					return HAL_NOT_AVAIL;
				}

			}
			else
			{
				(*len)=ind;
				ind=0;

				/* !! when the watch timer is installed, reset it over here */
				return HAL_NO_ERROR;
			}
		}
		else
		{
			/* !! when the watch timer is installed, reset it over here */
			return HAL_NOT_AVAIL;
		}
	}
	else
	{
		/* !! when the watch timer is installed, reset it over here */
		return HAL_FAIL;
	}
}

/* API to flush/reset the UART ring buffer
 *
 * PARAMETERS -
 * none
 *
 * RETURNS -
 * nothing
 */
void hal_uart_flush_buffer(void)
{
	cli();

	inPos=0;
	outPos=0;
	filled=0;

	sei();

	/* !! when the watch timer is installed, reset it over here */
}
