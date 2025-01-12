//
// These return values belong to the LED_MATRIX_GAME
//

#ifndef RETURN_VAL
#define RETURN_VAL

typedef enum
{
    SUCCESS = 0,
    GENERIC_ERROR,
    ALREADY_CONFIGURED,
    MEM_ALLOC_ERROR,
    NULL_DATA,

} return_val_t;

#endif
