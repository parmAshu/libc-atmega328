#ifndef HAL_DEFINITIONS
#define HAL_DEFINITIONS

/* 'Byte' alias for single byte values */
typedef uint8_t Byte;

/* Enumeration type for errors */
typedef enum hal_error{ HAL_NO_ERROR = 0, HAL_FAIL = 1, HAL_INV_PARAMS = 2, HAL_NOT_AVAIL = 3 }hal_error;

/* Enumeration type for parity types */
typedef enum hal_parity{ NO_PARITY = 'N', EVEN_PARITY = 'E', ODD_PARITY = 'O' }hal_parity;

#endif
