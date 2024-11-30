#ifndef GAME_LOGIC
#define GAME_LOGIC

#include "led_matrix_driver.h"
#include <inttypes.h>
#include "player_logic.h"
#include "shot_logic.h"

led_matrix_data_t *updateGame(led_matrix_data_t *asteroid_data, bool new_asteroid_data,
                              player_data_t *player_data, shot_data_t **shot_data_array, uint8_t shot_data_array_size);

#endif // GAME_LOGIC
