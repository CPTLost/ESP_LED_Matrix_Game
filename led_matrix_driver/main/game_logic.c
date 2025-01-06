#include "game_logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <esp_log.h>
#include <assert.h>
#include <limits.h>

#include "graphics.h"

#include "return_val.h"
#include "player_logic.h"
#include "led_matrix_driver.h"

#define HIT_POINTS 1
#define NUMBER_OF_POSSIBLE_COLLISION_POINTS 4 // This is determined by the player model and size

static const char *TAG = "GAME_LOGIC";
// static bool g_game_lost = false;
static uint8_t g_player_hit_counter = 0;

static uint16_t *shot_asteroids_indices = NULL;
static uint16_t amount_of_shot_asteroids_indices = 0;

static return_val_t fillReturnArrays(uint16_t combined_array_length, led_matrix_data_t *asteroid_data, uint16_t *ptr_index_array,
                                     uint8_t (*ptr_rgb_array)[3], shot_data_t **shot_data_array, uint8_t shot_data_array_size,
                                     uint16_t shot_data_indices_array_size, player_data_t *player_data, uint8_t collision_rgb_value[],
                                     uint8_t collision_counter, uint16_t collision_index_array[], bool game_lost)
{
    uint8_t shot_array_counter = 0;
    uint8_t shot_index_counter = 0;
    for (uint16_t i = 0; i < combined_array_length; i += 1)
    {
        /// inserting all asteroid indices
        if (i < asteroid_data->array_length)
        {
            ptr_index_array[i] = asteroid_data->ptr_index_array_leds_to_set[i];
            for (uint8_t j = 0; j < 3; j += 1)
            {
                ptr_rgb_array[i][j] = asteroid_data->ptr_rgb_array_leds_to_set[i][j];
            }
        }
        /// overwriting all shot asteroid indices with rgb values 0
        else if (i < asteroid_data->array_length + amount_of_shot_asteroids_indices)
        {
            ptr_index_array[i] = shot_asteroids_indices[i - asteroid_data->array_length];
            for (uint8_t j = 0; j < 3; j += 1)
            {
                ptr_rgb_array[i][j] = 0;
            }
        }
        /// inserting player data
        else if (i < asteroid_data->array_length + amount_of_shot_asteroids_indices + player_data->array_size)
        {
            uint8_t rgb_values_game_lost[3] = {25, 0, 0};
            ptr_index_array[i] = player_data->player_index_array[i - (asteroid_data->array_length + amount_of_shot_asteroids_indices)];
            for (uint8_t j = 0; j < 3; j += 1)
            {
                if (true == game_lost)
                {
                    ptr_rgb_array[i][j] = rgb_values_game_lost[j];
                }
                else
                {
                    ptr_rgb_array[i][j] = player_data->player_type->player_rgb_values[j];
                }
            }
        }
        /// inserting shot data
        else if (i < asteroid_data->array_length + amount_of_shot_asteroids_indices + player_data->array_size + shot_data_indices_array_size)
        {
            bool value_added = false;
            while (false == value_added && (shot_array_counter < shot_data_array_size))
            {
                if (shot_index_counter < shot_data_array[shot_array_counter]->array_size)
                {
                    if (false == (shot_data_array[shot_array_counter]->shot_hit_smth_array[shot_index_counter]))
                    {
                        ptr_index_array[i] = shot_data_array[shot_array_counter]->shot_index_array[shot_index_counter];
                        for (uint8_t j = 0; j < 3; j += 1)
                        {
                            ptr_rgb_array[i][j] = shot_data_array[shot_array_counter]->shot_type->shot_rgb_values[j];
                        }
                        value_added = true;
                    }
                    shot_index_counter += 1;
                }
                else
                {
                    shot_array_counter += 1;
                    if (shot_array_counter < shot_data_array_size)
                    {
                        shot_index_counter = 0;
                        if (false == (shot_data_array[shot_array_counter]->shot_hit_smth_array[shot_index_counter]))
                        {
                            ptr_index_array[i] = shot_data_array[shot_array_counter]->shot_index_array[shot_index_counter];
                            for (uint8_t j = 0; j < 3; j += 1)
                            {
                                ptr_rgb_array[i][j] = shot_data_array[shot_array_counter]->shot_type->shot_rgb_values[j];
                            }
                            value_added = true;
                        }
                        shot_index_counter += 1;
                    }
                }
            }
        }
    }
    for (uint8_t i = 0; i < collision_counter; i += 1)
    {
        ptr_index_array[combined_array_length - 1 - i] = collision_index_array[i];
        for (uint8_t j = 0; j < 3; j += 1)
        {
            ptr_rgb_array[combined_array_length - 1 - i][j] = collision_rgb_value[j];
        }
    }
    return SUCCESS;
}

static return_val_t updateShotAsteroidIndices(bool new_asteroid_data, led_matrix_data_t *asteroid_data)
{
    if (true == new_asteroid_data && 0 != amount_of_shot_asteroids_indices)
    {
        /// Decrement the previously shot asteroid indices (asteroids fall down) and NULLs expired indices
        uint16_t amount_of_expired_shot_asteroids_indices = 0;
        for (uint16_t i = 0; i < amount_of_shot_asteroids_indices; i += 1)
        {

            if (0 <= (shot_asteroids_indices[i] - MATRIX_SIDE_LENGTH))
            {
                shot_asteroids_indices[i] -= MATRIX_SIDE_LENGTH;
            }
            else
            {
                amount_of_expired_shot_asteroids_indices += 1;
                shot_asteroids_indices[i] = NULL;
            }
        }
        if (0 < (amount_of_shot_asteroids_indices - amount_of_expired_shot_asteroids_indices))
        {
            /// creating new array without the expired indices
            uint16_t *updated_shot_asteroids_indices;
            if (NULL == (updated_shot_asteroids_indices =
                             malloc(sizeof(*updated_shot_asteroids_indices) * (amount_of_shot_asteroids_indices - amount_of_expired_shot_asteroids_indices))))
            {
                ESP_LOGE(TAG, "Memory allocation failed while creating:\nupdated_shot_asteroids_indices\n");
                return MEM_ALLOC_ERROR;
            }
            /// looping through old array and copying valid data to new array
            uint16_t counter = 0;
            for (uint16_t i = 0; i < amount_of_shot_asteroids_indices; i += 1)
            {
                if (NULL != shot_asteroids_indices[i])
                {
                    updated_shot_asteroids_indices[counter] = shot_asteroids_indices[i];
                    counter += 1;
                }
            }
            free(shot_asteroids_indices);
            amount_of_shot_asteroids_indices -= amount_of_expired_shot_asteroids_indices;
            shot_asteroids_indices = updated_shot_asteroids_indices;
        }
        else
        {
            amount_of_shot_asteroids_indices = 0;
            free(shot_asteroids_indices);
            shot_asteroids_indices = NULL;
        }
    }
    return SUCCESS;
}

static return_val_t checkShotAsteroidCollision(bool set_asteroid_indices[], led_matrix_data_t *asteroid_data, shot_data_t **shot_data_array, uint8_t shot_data_array_size)
{
    if ((0 != shot_data_array_size) && (0 != asteroid_data->array_length))
    {
        uint16_t *new_shot_asteroids_indices;
        if (NULL == (new_shot_asteroids_indices = malloc(sizeof(*new_shot_asteroids_indices) * asteroid_data->array_length)))
        {
            ESP_LOGE(TAG, "Memory allocation failed while creating:\nnew_shot_asteroids_indices\n");
            return MEM_ALLOC_ERROR;
        }

        /// checks shot and asteroid collision and saves hit asteroid index
        uint8_t counter = 0;
        for (uint16_t i = 0; i < shot_data_array_size; i += 1)
        {
            for (uint16_t j = 0; j < shot_data_array[i]->array_size; j += 1)
            {
                if ((true == set_asteroid_indices[shot_data_array[i]->shot_index_array[j]]) &&
                    (false == shot_data_array[i]->shot_hit_smth_array[j]))
                {
                    new_shot_asteroids_indices[counter] = shot_data_array[i]->shot_index_array[j];
                    shot_data_array[i]->shot_hit_smth_array[j] = true;
                    // ESP_LOGW(TAG, "\n Hit smth !\n");
                    counter += 1;
                    set_asteroid_indices[shot_data_array[i]->shot_index_array[j]] = false;
                    amount_of_shot_asteroids_indices += 1;
                }
            }
        }

        if (0 < amount_of_shot_asteroids_indices)
        {
            if (NULL != shot_asteroids_indices)
            {
                if (NULL == (shot_asteroids_indices = realloc(shot_asteroids_indices, sizeof(*shot_asteroids_indices) * amount_of_shot_asteroids_indices)))
                {
                    ESP_LOGE(TAG, "Memory reallocation failed while reallocating:\nshot_asteroids_indices\n");
                    return MEM_ALLOC_ERROR;
                }
            }
            else
            {
                if (NULL == (shot_asteroids_indices = malloc(sizeof(*shot_asteroids_indices) * amount_of_shot_asteroids_indices)))
                {
                    ESP_LOGE(TAG, "Memory allocation failed while:\nshot_asteroids_indices\n");
                    return MEM_ALLOC_ERROR;
                }
            }
            /// copying new_shot_asteroids_indices into newly reallocated shot_asteroids_indices
            for (uint16_t i = 0; i < counter; i += 1)
            {
                shot_asteroids_indices[amount_of_shot_asteroids_indices - counter + i] = new_shot_asteroids_indices[i];
            }
            free(new_shot_asteroids_indices);
            new_shot_asteroids_indices = NULL;
            /// shot_asteroids_indices contains now all indices that must not be set later int the return data
        }
    }
    return SUCCESS;
}

/// @brief This function combines all data handles collisions, movement etc. and calculates all pixel that need to be set.
/// @brief IMPORTANT: the returned data was created with malloc -> the user must handle the deallocation
/// @param asteroid_data
/// @param new_asteroid_data
/// @param player_data
/// @param shot_data_array
/// @param shot_data_array_size
/// @param game_lost
/// @return led_matrix_data_t data which contains all indices and colors of every pixel that needs to be set. This data can be given to the updateLedMatrix(led_matrix_data_t *new_data) function as a parameter.
led_matrix_data_t *updateGame(led_matrix_data_t *asteroid_data, bool new_asteroid_data,
                              player_data_t *player_data, shot_data_t **shot_data_array, uint8_t shot_data_array_size, bool *game_lost)
{
    /// checks for valid inputs
    if (HIT_POINTS <= g_player_hit_counter)
    {
        *game_lost = true;
        g_player_hit_counter = 0;
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

    if ((NULL == shot_asteroids_indices) && (0 != asteroid_data->array_length))
    {
        shot_asteroids_indices = malloc(sizeof(*shot_asteroids_indices) * asteroid_data->array_length);
    }

    updateShotAsteroidIndices(new_asteroid_data, asteroid_data);

    /// Maps asteroids positions on a Matrix with bool values
    bool set_asteroid_indices[MATRIX_SIZE] = {0};
    for (uint16_t i = 0; i < (asteroid_data->array_length); i += 1)
    {
        set_asteroid_indices[asteroid_data->ptr_index_array_leds_to_set[i]] = true;
    }
    for (uint16_t i = 0; i < amount_of_shot_asteroids_indices; i += 1)
    {
        set_asteroid_indices[shot_asteroids_indices[i]] = false;
    }

    checkShotAsteroidCollision(set_asteroid_indices, asteroid_data, shot_data_array, shot_data_array_size);

    /// Increments all shot positions (shots go up)
    for (uint16_t i = 0; i < shot_data_array_size; i += 1)
    {
        for (uint16_t j = 0; j < shot_data_array[i]->array_size; j += 1)
        {
            if (false == shot_data_array[i]->shot_hit_smth_array[j])
            {
                /// checks if shot is out of bounds, when true shot is marked as "hit_smth"
                if (MATRIX_SIZE > (shot_data_array[i]->shot_index_array[j] + MATRIX_SIDE_LENGTH))
                {
                    shot_data_array[i]->shot_index_array[j] += MATRIX_SIDE_LENGTH;
                }
                else
                {
                    shot_data_array[i]->shot_hit_smth_array[j] = true;
                }
            }
        }
    }

    checkShotAsteroidCollision(set_asteroid_indices, asteroid_data, shot_data_array, shot_data_array_size);

    /// compares asteroid positions with player position -> looks for collisions
    uint16_t collision_index_array[NUMBER_OF_POSSIBLE_COLLISION_POINTS] = {0};
    uint8_t collision_counter = 0;
    for (uint16_t i = 0; i < (player_data->array_size); i += 1)
    {
        if (true == set_asteroid_indices[player_data->player_index_array[i]])
        {
            g_player_hit_counter += 1;
            collision_index_array[collision_counter] = player_data->player_index_array[i];
            collision_counter += 1;
        }
    }

    /// getting the sum of all array sizes of the shots
    uint16_t shot_data_indices_array_size = 0;
    for (uint16_t i = 0; i < shot_data_array_size; i += 1)
    {
        for (uint16_t j = 0; j < shot_data_array[i]->array_size; j += 1)
        {
            if (false == shot_data_array[i]->shot_hit_smth_array[j])
            {
                shot_data_indices_array_size += 1;
            }
        }
    }

    assert(collision_counter <= NUMBER_OF_POSSIBLE_COLLISION_POINTS);
    assert(asteroid_data->array_length +
               player_data->array_size +
               shot_data_indices_array_size +
               collision_counter +
               amount_of_shot_asteroids_indices <
           UINT16_MAX);

    /// Creating led_matrix_data to return
    uint16_t combined_array_length = asteroid_data->array_length +
                                     player_data->array_size +
                                     shot_data_indices_array_size +
                                     collision_counter +
                                     amount_of_shot_asteroids_indices;

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

    uint8_t collision_rgb_value[3] = {30, 5, 20};

    fillReturnArrays(combined_array_length, asteroid_data, ptr_index_array, ptr_rgb_array, shot_data_array,
                     shot_data_array_size, shot_data_indices_array_size, player_data, collision_rgb_value,
                     collision_counter, collision_index_array, *game_lost);

    matrix_data->ptr_index_array_leds_to_set = ptr_index_array;
    matrix_data->ptr_rgb_array_leds_to_set = ptr_rgb_array;
    matrix_data->array_length = combined_array_length;

    return matrix_data;
}