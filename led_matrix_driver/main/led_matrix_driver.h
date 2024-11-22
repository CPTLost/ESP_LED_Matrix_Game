#ifndef LED_MATRIX_DRIVER
#define LED_MATRIX_DRIVER

#include "return_val.h"
#include <inttypes.h>

/// Selection of the used led matrix hardware
// #define MATRIX_32X32
#define MATRIX_16X16
// #define MATRIX_5X5

#ifdef MATRIX_5X5
#define MATRIX_SIZE 5 * 5
#define MATRIX_SIDE_LENGTH 5
#define MAX_INDEX 24
#elif defined MATRIX_16X16
#define MATRIX_SIZE 16 * 16
#define MATRIX_SIDE_LENGTH 16
#define MAX_INDEX 255
#elif defined MATRIX_32X32
#define MATRIX_SIZE 32 * 32
#define MATRIX_SIDE_LENGTH 32
#define MAX_INDEX 1023
#else
#error "Unknown MATRIX_SIZE"
#endif
typedef struct
{
    uint16_t *ptr_index_array_leds_to_set;
    uint8_t (*ptr_rgb_array_leds_to_set)[3];
    // Both arrays above must be the same length !!!
    uint16_t array_length;
} led_matrix_data_t;

return_val_t initLedMatrix(void);
return_val_t updateLedMatrix(led_matrix_data_t *new_data);

return_val_t test();

#endif