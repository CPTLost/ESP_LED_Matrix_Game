#ifndef SHOT_LOGIC
#define SHOT_LOGIC

#include "led_matrix_driver.h"
#include <inttypes.h>
#include <stdbool.h>

#include "player_logic.h"

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

typedef struct
{
    const uint8_t *shot_shape_array;
    const uint8_t *shot_rgb_values;
    const uint8_t array_size;
} shot_t;

typedef struct
{
    shot_t *shot_type;
    uint16_t *shot_index_array;
    bool *shot_hit_smth_array;
    uint8_t array_size;
} shot_data_t;

extern const shot_t normal_shot_type;
extern const shot_t double_shot_type;

shot_data_t *createShot(player_data_t *player_data, shot_t *shot_type);
return_val_t updatedShotDataArray(shot_data_t *new_shot, shot_data_t ***shot_data_array, uint8_t shot_data_array_size);

#endif