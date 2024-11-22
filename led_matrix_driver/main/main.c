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

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

static const char *TAG_MAIN = "APP_MAIN";

void app_main(void)
{
    initLedMatrix();
    while (1)
    {

        // circular_linked_list_test();

        uint16_t index_array[] = {0, 1, 2, 1 + MATRIX_SIDE_LENGTH};
        uint8_t rgb_array[][3] = {{0, 0, 25}, {0, 0, 25}, {0, 0, 25}, {0, 0, 25}};

        led_matrix_data_t new_test_player_data = {.ptr_index_array_leds_to_set = index_array,
                                                  .ptr_rgb_array_leds_to_set = rgb_array,
                                                  .array_length = ARRAY_LENGTH(index_array)};

        // printf("\nrgb = %d\n", new_data.ptr_rgb_array_leds_to_set[2][2]);
        // printf("\narray_length = %d\n", new_data.array_length);

        led_matrix_data_t *new_asteroid_data = updateAsteroidField();
        led_matrix_data_t *new_update_data = updateGame(new_asteroid_data, &new_test_player_data, 0);

        if (new_update_data != MEM_ALLOC_ERROR)
        {
            // test();

            // printf("new update\n");
            // printf("array length = %d\n", new_update_data->array_length);
            // for (int i = 0; i < new_update_data->array_length; i += 1)
            // {
            //     printf("new_asteroid_data->array_index[%d] = %d\n", i, new_update_data->ptr_index_array_leds_to_set[i]);
            // }

            updateLedMatrix(new_update_data);

            /// these must be freed and this must be done before appending new ptr to the global buffers
            free(new_asteroid_data->ptr_index_array_leds_to_set);
            free(new_asteroid_data->ptr_rgb_array_leds_to_set);
            free(new_asteroid_data);

            free(new_update_data->ptr_index_array_leds_to_set);
            free(new_update_data->ptr_rgb_array_leds_to_set);
            free(new_update_data);

            // ESP_LOGW(TAG_MAIN, "Random Number = %ld\n", esp_random() % 1000);}
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
