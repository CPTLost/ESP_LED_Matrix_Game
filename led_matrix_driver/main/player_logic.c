#include "player_logic.h"

#include <inttypes.h>
#include <stdlib.h>
#include <esp_log.h>

#include "led_matrix_driver.h"

static const char *TAG = "PLAYER_LOGIC";

static const uint8_t normal_player_shape_array[] = {3, 1};
static const uint8_t std_player_rgb_values[] = {0, 0, 25};
const player_t normal_player = {.shape_array = normal_player_shape_array,
                                .player_rgb_values = std_player_rgb_values,
                                .array_size = ARRAY_LENGTH(normal_player_shape_array)};

static void fillPlayerData(player_data_t *player_data)
{
    if (5 == MATRIX_SIDE_LENGTH)
    {
        ESP_LOGE(TAG, "Normal player creation for Matrix side length of 5 is not supported\n");
        return;
    }
    uint16_t start_position = MATRIX_SIDE_LENGTH / 2;
    player_data->shot_start_position = start_position + MATRIX_SIDE_LENGTH * player_data->player_type->array_size;

    uint8_t values_added_counter = 0;

    uint8_t *shape_array = player_data->player_type->shape_array;

    for (uint8_t i = 0; i < player_data->player_type->array_size; i += 1)
    {
        if (0 == shape_array[i] % 2)
        {
            // gerade Anzahl
            for (int8_t j = -(shape_array[i] / 2); j < (shape_array[i] / 2); j += 1)
            {
                player_data->player_index_array[values_added_counter] = start_position + j + i * MATRIX_SIDE_LENGTH;
                values_added_counter += 1;
            }
        }
        else
        {
            // ungerade Anzahl
            for (int8_t j = -(shape_array[i] / 2); j <= (shape_array[i] / 2); j += 1)
            {
                player_data->player_index_array[values_added_counter] = start_position + j + i * MATRIX_SIDE_LENGTH;
                values_added_counter += 1;
            }
        }
    }
}

player_data_t *initPlayer(player_t *player_type)
{
    /// creating return data container
    player_data_t *player_data = malloc(sizeof(player_data_t));
    if (NULL == player_data)
    {
        ESP_LOGE(TAG, "Memory allocation failed while creating:\nnormal_player_data\n");
        return NULL;
    }

    /// calc index array size
    uint8_t player_index_array_size = 0;
    for (uint8_t i = 0; i < player_type->array_size; i += 1)
    {
        player_index_array_size += player_type->shape_array[i];
    }

    /// creating player index array container
    uint16_t *player_index_array = malloc(sizeof(uint16_t) * player_index_array_size);
    if (NULL == player_index_array)
    {
        ESP_LOGE(TAG, "Memory allocation failed while creating:\nplayer_index_array\n");
        free(player_data);
        return NULL;
    }

    player_data->array_size = player_index_array_size;
    player_data->player_index_array = player_index_array;
    player_data->player_type = player_type;

    fillPlayerData(player_data);

    return player_data;
}