#include "asteroid_logic.h"

#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_random.h>
#include <stdlib.h>

#include "led_matrix_driver.h"

/*Maybe put into SDK config in the main project?*/

/// Selection of the used led matrix hardware
// #define MATRIX_32X32
// #define MATRIX_16X16
#define MATRIX_5X5

#ifdef MATRIX_5X5
#define MATRIX_SIDE_LENGTH 5
#define MAX_INDEX 24
#endif
#ifdef MATRIX_16X16
#define MATRIX_SIDE_LENGTH 16
#define MAX_INDEX 255
#endif
#ifdef MATRIX_32X32
#define MATRIX_SIDE_LENGTH 32
#define MAX_INDEX 1023
#endif

static const char *TAG = "ASTEROID_LOGIC";

static uint8_t asteroids_per_row = 1;
static uint16_t *asteroid_field_indices = NULL;
static uint16_t field_size = 0;

rtrn_val_t updateAsteroidField()
{
    if ((field_size / asteroids_per_row) < MATRIX_SIDE_LENGTH)
    {
        field_size += asteroids_per_row;
    }
    uint16_t start_index_array[asteroids_per_row];

    if (NULL == asteroid_field_indices)
    {
        /// In the first run of the update function the asteroid field must be created first
        if (NULL == (asteroid_field_indices = malloc(sizeof(start_index_array) * asteroids_per_row)))
        {
            ESP_LOGE(TAG, "Memory allocation failed!\n");
            return MEM_ALLOC_ERROR;
        }

        for (uint8_t i = 0; i < field_size; i += 1)
        {
            asteroid_field_indices[i] = MAX_INDEX - esp_random() % MATRIX_SIDE_LENGTH;
        }
    }
    else
    {
        /// Every run of the update function runs follwoing code to check for a gap and updates the asteroid_field_indices
        /// Following while loops checks if the random index is usable
        bool start_index_okay = false;
        while (false == start_index_okay)
        {
            /// creates random start indices
            for (uint8_t i = 0; i < asteroids_per_row; i += 1)
            {
                /// The esp_random() automatically waits until enough entropy has gathered to return a new random number
                start_index_array[i] = MAX_INDEX - esp_random() % MATRIX_SIDE_LENGTH;
            }
            /// checks for necessary gaps for the player to go through
            /// first for loop loops through rows
            for (uint16_t k = 0; k < (field_size / asteroids_per_row); k += asteroids_per_row)
            {
                /// checks if new start index overlaps with previous index of this row
                for (uint16_t i = 0; i < asteroids_per_row; i += 1)
                {
                    for (uint16_t j = 0; j < asteroids_per_row; j += 1)
                    {
                        if (asteroid_field_indices[i + k] != start_index_array[j])
                        {
                            // no overlaps in current row but i dont need to check for every row
                            // and how do i check for a valdi path???
                        }
                    }
                }
            }
        }
        /// In this loop the asteroids are shifted into the next row (its minus coz its from top to bottom)
        for (uint16_t i = 0; i < field_size - asteroids_per_row; i += 1)
        {
            asteroid_field_indices[i] -= MATRIX_SIDE_LENGTH;
        }
        /// The asteroid_field_indices will now get the new generated valid indices appended
        if (NULL == (asteroid_field_indices = realloc(asteroid_field_indices, sizeof(start_index_array) * field_size)))
        {
            ESP_LOGE(TAG, "Memory reallocation failed!\n");
            return MEM_ALLOC_ERROR;
        }
        for (uint16_t i = 0; i < asteroids_per_row; i += 1)
        {
            // puts new data at the end of the array id khow good that is .....
            // when max rows is reached it certainly must overwrite the oldest, implementation?
            asteroid_field_indices[field_size - asteroids_per_row + i] = start_index_array[i];
        }
    }

    return SUCCESS;
}