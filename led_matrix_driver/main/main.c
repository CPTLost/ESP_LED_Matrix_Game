#include <stdio.h>
#include <inttypes.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <esp_random.h>

#include "led_strip.h"

#include "rtrn_val.h"
#include "led_matrix_driver.h"

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

static const char *TAG_MAIN = "APP_MAIN";

void app_main(void)
{
    initLedMatrix();
    while (1)
    {
        uint16_t index_array[] = {1, 2, 4, 9, 3};
        uint8_t rgb_array[][3] = {{25, 0, 0}, {0, 25, 0}, {0, 0, 25}, {10, 10, 10}, {0, 0, 50}};

        led_matrix_data_t new_data = {.ptr_index_array_leds_to_set = index_array,
                                      .ptr_rgb_array_leds_to_set = rgb_array,
                                      .array_length = ARRAY_LENGTH(index_array)};

        printf("\nrgb = %d\n", new_data.ptr_rgb_array_leds_to_set[2][2]);
        printf("\narray_length = %d\n", new_data.array_length);

        updateLedMatrix(&new_data);

        ESP_LOGW(TAG_MAIN, "Random Number = %ld\n", esp_random() % 1000);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
