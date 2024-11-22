#ifndef GAME_LOGIC
#define GAME_LOGIC

#include "led_matrix_driver.h"
#include <inttypes.h>

led_matrix_data_t *updateGame(led_matrix_data_t *asteroid_data, led_matrix_data_t *player_data, uint8_t shot_fired_flag);

#endif // GAME_LOGIC
