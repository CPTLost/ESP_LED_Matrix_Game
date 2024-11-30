#include <stdio.h>
#include <inttypes.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <esp_random.h>

#include "led_strip.h"

#include "return_val.h"
#include "led_matrix_driver.h"
#include "asteroid_logic.h"
#include "game_logic.h"
#include "player_logic.h"
#include "shot_logic.h"

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

static const char *TAG_MAIN = "APP_MAIN";

#define TEMP_NUMBER_OF_SHOTS 1
static shot_data_t **g_shot_data_array = NULL;
static uint8_t g_shot_data_array_size = TEMP_NUMBER_OF_SHOTS;

void app_main(void)
{
    if (NULL == (g_shot_data_array = malloc(sizeof(*g_shot_data_array) * TEMP_NUMBER_OF_SHOTS)))
    {
        ESP_LOGE(TAG_MAIN, "Memalloc failed\n");
    }

    /// create function for player not implemented yet
    uint16_t player_index_array[] = {7, 8, 9, 8 + MATRIX_SIDE_LENGTH};
    uint8_t player_shot_start_pos = 8;
    player_data_t normal_player_data = {.player_type = &normal_player,
                                        .shot_start_position = player_shot_start_pos,
                                        .player_index_array = player_index_array,
                                        .array_size = ARRAY_LENGTH(player_index_array)};

    shot_data_t *new_shot_data = createShot(&normal_player_data, &normal_shot_type);
    g_shot_data_array[0] = new_shot_data;

    initLedMatrix();
    while (1)
    {
        /// Testing shot logic

        led_matrix_data_t *new_asteroid_data = updateAsteroidField();

        led_matrix_data_t *new_matrix_data = updateGame(new_asteroid_data, true, &normal_player_data, g_shot_data_array, g_shot_data_array_size);
        updateLedMatrix(new_matrix_data);

        free(new_asteroid_data->ptr_index_array_leds_to_set);
        free(new_asteroid_data->ptr_rgb_array_leds_to_set);
        free(new_asteroid_data);

        free(new_matrix_data->ptr_index_array_leds_to_set);
        free(new_matrix_data->ptr_rgb_array_leds_to_set);
        free(new_matrix_data);

        for (uint8_t i = 0; i < g_shot_data_array_size; i += 1)
        {
            bool hit_smth = true;
            for (uint8_t j = 0; j < g_shot_data_array[i]->array_size; j += 1)
            {
                if (false == g_shot_data_array[i]->shot_hit_smth_array[j])
                {
                    hit_smth = false;
                }
            }
            if (true == hit_smth)
            {
                free(g_shot_data_array[i]);
                // ESP_LOGW(TAG_MAIN, "NEW SHOT!\n");
                g_shot_data_array[i] = createShot(&normal_player_data, &normal_shot_type);
            }
        }

        // uint16_t index_array[] = {0, 1, 2, 1 + MATRIX_SIDE_LENGTH};
        // uint8_t rgb_array[][3] = {{0, 0, 25}, {0, 0, 25}, {0, 0, 25}, {0, 0, 25}};

        // led_matrix_data_t new_test_player_data = {.ptr_index_array_leds_to_set = index_array,
        //                                           .ptr_rgb_array_leds_to_set = rgb_array,
        //                                           .array_length = ARRAY_LENGTH(index_array)};

        // uint16_t temp_player_index_array[] = {7, 8, 9, 8 + MATRIX_SIDE_LENGTH};
        // uint8_t temp_player_shot_start_pos = 8 + MATRIX_SIDE_LENGTH * 2;

        // player_data_t normal_player_data = {.player_type = &normal_player,
        //                                     .shot_start_position = temp_player_shot_start_pos,
        //                                     .player_index_array = temp_player_index_array,
        //                                     .array_size = ARRAY_LENGTH(temp_player_index_array)};

        // shot_data_t *new_shot = createShot(&normal_player_data, &normal_shot_type);

        // led_matrix_data_t new_shot_matrix_data = {.array_length = new_shot->array_size,
        //                                           .ptr_index_array_leds_to_set = new_shot->shot_index_array,
        //                                           .ptr_rgb_array_leds_to_set = new_shot->shot_type->shot_rgb_values};

        // led_matrix_data_t new_player_matrix_data = {.array_length = normal_player_data.array_size,
        //                                             .ptr_index_array_leds_to_set = normal_player_data.player_index_array,
        //                                             .ptr_rgb_array_leds_to_set = rgb_array};

        // updateLedMatrix(&new_shot_matrix_data);
        // updateLedMatrix(&new_player_matrix_data);

        // free(new_shot->shot_index_array);
        // free(new_shot);

        // printf("\nrgb = %d\n", new_data.ptr_rgb_array_leds_to_set[2][2]);
        // printf("\narray_length = %d\n", new_data.array_length);

        // led_matrix_data_t *new_asteroid_data = updateAsteroidField();
        // led_matrix_data_t *new_update_data = updateGame(new_asteroid_data, &new_test_player_data, 0);

        // if (new_update_data != MEM_ALLOC_ERROR)
        // {
        // test();

        // printf("new update\n");
        // printf("array length = %d\n", new_update_data->array_length);
        // for (int i = 0; i < new_update_data->array_length; i += 1)
        // {
        //     printf("new_asteroid_data->array_index[%d] = %d\n", i, new_update_data->ptr_index_array_leds_to_set[i]);
        // }

        // updateLedMatrix(new_update_data);

        // /// these must be freed and this must be done before appending new ptr to the global buffers
        // free(new_asteroid_data->ptr_index_array_leds_to_set);
        // free(new_asteroid_data->ptr_rgb_array_leds_to_set);
        // free(new_asteroid_data);

        // free(new_update_data->ptr_index_array_leds_to_set);
        // free(new_update_data->ptr_rgb_array_leds_to_set);
        // free(new_update_data);

        // }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
