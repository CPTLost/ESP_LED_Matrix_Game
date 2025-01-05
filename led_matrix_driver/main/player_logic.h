#ifndef PLAYER_LOGIC
#define PLAYER_LOGIC

#include <inttypes.h>

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

typedef struct
{
    uint8_t *shape_array;
    uint8_t array_size;
    uint8_t *player_rgb_values;
} player_t;

typedef struct
{
    player_t *player_type;
    uint16_t *player_index_array;
    uint16_t shot_start_position;
    uint8_t array_size;
} player_data_t;

extern const player_t normal_player;

player_data_t *initPlayer(player_t *player_type);

#endif