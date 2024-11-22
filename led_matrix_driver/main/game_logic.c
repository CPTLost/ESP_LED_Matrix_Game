#include "game_logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <esp_log.h>
#include <assert.h>

#define NUMBER_OF_POSSIBLE_COLLISION_POINTS 4 // This is determined by the player model and size

static const char *TAG = "GAME_LOGIC";
static bool g_game_lost = false;

led_matrix_data_t *updateGame(led_matrix_data_t *asteroid_data, led_matrix_data_t *player_data, uint8_t shot_fired_flag)
{
    if (true == g_game_lost)
    {
        ESP_LOGI(TAG, "GAME OVER\n");
        g_game_lost = false;
        // Show GAME OVER and survived time etc.
    }
    if (NULL == asteroid_data)
    {
        ESP_LOGE(TAG, "NULL Asteroid Data\n");
        return NULL_DATA;
    }
    if (NULL == player_data)
    {
        ESP_LOGE(TAG, "NULL Player Data\n");
        return NULL_DATA;
    }

    bool set_asteroid_indices[MATRIX_SIZE] = {0};
    uint16_t collision_index_array[NUMBER_OF_POSSIBLE_COLLISION_POINTS] = {0};
    uint8_t collision_counter = 0;
    for (uint16_t i = 0; i < (asteroid_data->array_length); i += 1)
    {
        set_asteroid_indices[asteroid_data->ptr_index_array_leds_to_set[i]] = true;
    }
    for (uint16_t i = 0; i < (player_data->array_length); i += 1)
    {
        if (true == set_asteroid_indices[player_data->ptr_index_array_leds_to_set[i]])
        {
            g_game_lost = true;
            collision_index_array[collision_counter] = player_data->ptr_index_array_leds_to_set[i];
            collision_counter += 1;
        }
    }

    assert(collision_counter <= NUMBER_OF_POSSIBLE_COLLISION_POINTS);

    // temp //
    bool shot_active = false;
    uint8_t size_of_shot = 2;
    /////////

    uint16_t combined_array_length = asteroid_data->array_length + player_data->array_length +
                                     ((shot_active) ? size_of_shot : 0) + ((0 != collision_counter) ? collision_counter : 0);

    led_matrix_data_t *matrix_data;
    uint16_t *ptr_index_array;
    uint8_t(*ptr_rgb_array)[3];

    if (NULL == (ptr_index_array = malloc(sizeof(*ptr_index_array) * combined_array_length)) ||
        NULL == (ptr_rgb_array = malloc(sizeof(**ptr_rgb_array) * sizeof(*ptr_rgb_array) * combined_array_length)) ||
        NULL == (matrix_data = malloc(sizeof(*matrix_data))))
    {
        ESP_LOGE(TAG, "Memory allocation failed while creating:\nptr_index_array\nptr_rgb_array\nmatrix_data");
        return MEM_ALLOC_ERROR;
    }

    for (uint16_t i = 0; i < combined_array_length; i += 1)
    {
        if (i < asteroid_data->array_length)
        {
            ptr_index_array[i] = asteroid_data->ptr_index_array_leds_to_set[i];
            for (uint8_t j = 0; j < 3; j += 1)
            {
                ptr_rgb_array[i][j] = asteroid_data->ptr_rgb_array_leds_to_set[i][j];
            }
        }
        else if (i < asteroid_data->array_length + player_data->array_length)
        {
            ptr_index_array[i] = player_data->ptr_index_array_leds_to_set[i - asteroid_data->array_length];
            for (uint8_t j = 0; j < 3; j += 1)
            {
                ptr_rgb_array[i][j] = player_data->ptr_rgb_array_leds_to_set[i - asteroid_data->array_length][j];
            }
        }

        ////////// SHOT DATA MUST LATER BE ADDED HERE/////////
    }

    uint8_t collision_rgb_value[3] = {23, 13, 23};

    for (uint8_t i = 0; i < collision_counter; i += 1)
    {
        ptr_index_array[combined_array_length - 1 - i] = collision_index_array[i];
        for (uint8_t j = 0; j < 3; j += 1)
        {
            ptr_rgb_array[combined_array_length - 1 - i][j] = collision_rgb_value[j];
        }
    }

    matrix_data->ptr_index_array_leds_to_set = ptr_index_array;
    matrix_data->ptr_rgb_array_leds_to_set = ptr_rgb_array;
    matrix_data->array_length = combined_array_length;

    // ESP_LOGW(TAG, "asteroid data length = %d", asteroid_data->array_length);
    // ESP_LOGW(TAG, "player data length = %d", player_data->array_length);
    // ESP_LOGW(TAG, "collision counter = %d", collision_counter);
    // ESP_LOGW(TAG, "combined_array_length = %d", combined_array_length);
    // ESP_LOGW(TAG, "matrix_data array length = %d\n", matrix_data->array_length);

    /// THE USER MUST DEALLOCATE THE RETURNED DATA WHEN NOT USED ANYMORE
    return matrix_data;
}