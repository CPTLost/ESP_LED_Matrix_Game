#ifndef LED_MATRIX_DRIVER
#define LED_MATRIX_DRIVER

#include "return_val.h"
#include <inttypes.h>

typedef struct
{
    uint16_t *ptr_index_array_leds_to_set;
    uint8_t (*ptr_rgb_array_leds_to_set)[3];
    // Both arrays above must be the same length !!!
    uint16_t array_length;
} led_matrix_data_t;

return_val_t initLedMatrix(void);
return_val_t updateLedMatrix(led_matrix_data_t *new_data);

#endif