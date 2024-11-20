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

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

static const char *TAG_MAIN = "APP_MAIN";

void app_main(void)
{
    initLedMatrix();
    while (1)
    {

        // circular_linked_list_test();

        uint16_t index_array[] = {1, 2, 4, 9, 7};
        uint8_t rgb_array[][3] = {{25, 0, 0}, {0, 25, 0}, {0, 0, 25}, {10, 10, 10}, {0, 0, 50}};

        led_matrix_data_t new_data = {.ptr_index_array_leds_to_set = index_array,
                                      .ptr_rgb_array_leds_to_set = rgb_array,
                                      .array_length = ARRAY_LENGTH(index_array)};

        // printf("\nrgb = %d\n", new_data.ptr_rgb_array_leds_to_set[2][2]);
        // printf("\narray_length = %d\n", new_data.array_length);

        led_matrix_data_t *new_asteroid_data = updateAsteroidField();
        if (new_asteroid_data != MEM_ALLOC_ERROR)
        {

            // updateLedMatrix(&new_data);
            updateLedMatrix(new_asteroid_data);
            printf("new update\n");
            printf("array length = %d\n", new_asteroid_data->array_length);
            for (int i = 0; i < new_asteroid_data->array_length; i += 1)
            {
                printf("new_asteroid_data->array_index[%d] = %d\n", i, new_asteroid_data->ptr_index_array_leds_to_set[i]);
            }

            // free(new_asteroid_data->ptr_index_array_leds_to_set);
            // free(new_asteroid_data->ptr_rgb_array_leds_to_set);
            // free(new_asteroid_data);

            // ESP_LOGW(TAG_MAIN, "Random Number = %ld\n", esp_random() % 1000);}
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
