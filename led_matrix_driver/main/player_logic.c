#include "player_logic.h"

static const uint8_t normal_player_shape_array[] = {3, 1};
static const uint8_t std_player_rgb_values[] = {0, 0, 25};
const player_t normal_player = {.shape_array = normal_player_shape_array,
                                .player_rgb_values = std_player_rgb_values,
                                .array_size = ARRAY_LENGTH(normal_player_shape_array)};