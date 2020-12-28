#ifndef HAL_UART
#define HAL_UART

#include "hal_definitions.h"

/* configure the maximum UART ring buffer size here (in bytes); Do not use values larger than 254 */
#define MAX_HAL_UART_BUFF_SIZE 200

/* structure for configuring the UART port*/
typedef struct hal_uart_config{ uint16_t _baud; hal_parity _parity; uint8_t _stopBits; uint8_t _buffSize;}hal_uart_config;

hal_error hal_uart_driver_install( hal_uart_config);

hal_error hal_uart_driver_uninstall( void );

hal_error hal_uart_write( Byte );

hal_error hal_uart_write_bytes( Byte *, uint32_t );

hal_error hal_uart_print( Byte * );

hal_error hal_uart_println( Byte * );

hal_error hal_uart_write_bytes_nl( Byte *, uint32_t );

hal_error hal_uart_get_byte(Byte *);

hal_error hal_uart_get_bytes_num(Byte *, uint8_t );

hal_error hal_uart_get_bytes_until(Byte *, uint32_t * , Byte);

void hal_uart_flush_buffer(void);

#endif
