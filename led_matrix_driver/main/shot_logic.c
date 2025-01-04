#include "shot_logic.h"

#include <stdio.h>
#include <esp_log.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include <driver/gpio.h>

#include "return_val.h"

static const uint8_t normal_shot_shape_array[] = {1};
static const uint8_t std_shot_rgb_values[] = {0, 25, 0};
const shot_t normal_shot_type = {.shot_shape_array = normal_shot_shape_array,
                                 .shot_rgb_values = std_shot_rgb_values,
                                 .array_size = ARRAY_LENGTH(normal_shot_shape_array)};

static const uint8_t double_shot_shape_array[] = {1, 1};
const shot_t double_shot_type = {.shot_shape_array = double_shot_shape_array,
                                 .shot_rgb_values = std_shot_rgb_values,
                                 .array_size = ARRAY_LENGTH(double_shot_shape_array)};

static const char *TAG = "SHOT_LOGIC";

static bool isr_installed = false;

void trigger_shot(void);
void trigger_isr_handler()
{
    trigger_shot();
}

return_val_t init_button_for_shot_trigger(uint8_t GPIO_BUTTON_PIN)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1 << GPIO_BUTTON_PIN,
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    if (isr_installed == false)
    {
        gpio_install_isr_service(0);
        isr_installed = true;
    }

    gpio_isr_handler_add(GPIO_BUTTON_PIN, trigger_isr_handler, NULL);

    return SUCCESS;
}

__attribute__((weak)) void trigger_shot(void) {}

shot_data_t *createShot(player_data_t *player_data, shot_t *shot_type)
{
    if (NULL == player_data)
    {
        ESP_LOGE(TAG, "player_data == NULL\n");
        return (shot_data_t *)NULL_DATA;
    }
    if (NULL == shot_type)
    {
        ESP_LOGE(TAG, "shot_type == NULL\n");
        return (shot_data_t *)NULL_DATA;
    }

    shot_data_t *new_shot_data;
    uint16_t *shot_index_array;
    bool *shot_hit_smth_array;

    /// Get shot index array size from shot_type->shape
    uint8_t shot_index_array_size = 0;
    for (uint8_t i = 0; i < shot_type->array_size; i += 1)
    {
        shot_index_array_size += shot_type->shot_shape_array[i];
    }

    if (NULL == (new_shot_data = malloc(sizeof(*new_shot_data))) ||
        NULL == (shot_index_array = malloc(sizeof(*shot_index_array) * shot_index_array_size)) ||
        NULL == (shot_hit_smth_array = malloc(sizeof(*shot_hit_smth_array) * shot_index_array_size)))
    {
        ESP_LOGE(TAG, "Memory allocation failed while creating:\nnew_shot_data\nshot_index_array\nshot_hit_smth_array\n");
        return (shot_data_t *)MEM_ALLOC_ERROR;
    }

    /// Function to generate the pixels according to the shot shape beginning at the player_data->start_positions_shots
    uint8_t values_added_counter = 0;
    for (uint8_t i = 0; i < shot_type->array_size; i += 1)
    {
        uint8_t current_shot_shape_value = shot_type->shot_shape_array[i];
        uint8_t current_row_offset = player_data->shot_start_position / MATRIX_SIDE_LENGTH;
        shot_index_array[values_added_counter] = player_data->shot_start_position + MATRIX_SIDE_LENGTH * i;
        values_added_counter += 1;
        if (0 == current_shot_shape_value % 2)
        {
            for (uint8_t j = 0; j < (current_shot_shape_value / 2); j += 1)
            {
                shot_index_array[values_added_counter] =
                    (MATRIX_SIDE_LENGTH + MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i >=
                     player_data->shot_start_position + MATRIX_SIDE_LENGTH * i + j + 1)
                        ? player_data->shot_start_position + MATRIX_SIDE_LENGTH * i + j + 1
                        : MATRIX_SIDE_LENGTH + MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i;
                values_added_counter += 1;
            }
            for (uint8_t j = 1; j < (current_shot_shape_value / 2); j += 1)
            {
                shot_index_array[values_added_counter] =
                    (MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i <
                     player_data->shot_start_position + MATRIX_SIDE_LENGTH * i - j)
                        ? player_data->shot_start_position + MATRIX_SIDE_LENGTH * i - j
                        : MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i;
                values_added_counter += 1;
            }
        }
        else
        { //////// needs to be looked at again
            for (uint8_t j = 0; j < (current_shot_shape_value / 2); j += 1)
            {
                shot_index_array[values_added_counter] =
                    (MATRIX_SIDE_LENGTH + MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i >=
                     player_data->shot_start_position + MATRIX_SIDE_LENGTH * i + j + 1)
                        ? player_data->shot_start_position + MATRIX_SIDE_LENGTH * i + j + 1
                        : MATRIX_SIDE_LENGTH + MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i;
                values_added_counter += 1;
                shot_index_array[values_added_counter] =
                    (MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i <
                     player_data->shot_start_position + MATRIX_SIDE_LENGTH * i - j)
                        ? player_data->shot_start_position + MATRIX_SIDE_LENGTH * i - j
                        : MATRIX_SIDE_LENGTH * current_row_offset + MATRIX_SIDE_LENGTH * i;
                values_added_counter += 1;
            }
        }
    }
    // ESP_LOGW(TAG, "values_added_counter = %d\nshot_index_array_size = %d\n", values_added_counter, shot_index_array_size);
    assert(values_added_counter == shot_index_array_size);

    for (uint16_t i = 0; i < shot_index_array_size; i += 1)
    {
        shot_hit_smth_array[i] = false;
    }

    new_shot_data->shot_index_array = shot_index_array;
    new_shot_data->shot_hit_smth_array = shot_hit_smth_array;
    new_shot_data->array_size = shot_index_array_size;
    new_shot_data->shot_type = shot_type;

    return new_shot_data;
}
// This created shot is then appended to a global shot array.
// The GL should be called by a task notify as long as the shot array isnt empty so it can update the shots

/// @brief updates shot_data_array: deletes expired shots and creates new array containing the given new_shot and all old valid shots.
/// @param new_shot ptr to new shot
/// @param shot_data_array ptr to shot data array
/// @param ptr_shot_data_array_size ptr to shot_data_array_size
/// @return SUCCESS
return_val_t updatedShotDataArray(shot_data_t *new_shot, shot_data_t ***shot_data_array, uint8_t *ptr_shot_data_array_size)
{
    // situations: new shot available or not, Shot data array is empty or not -> 4 different cases
    // 1. if new shot and shot data array is empty -> just return
    // 2. if new shot is empty and data array is not -> check for expired shots and delete them
    // 3. if new shot is not empty and data array is empty -> create new shot data array with new shot
    // 4. if new shot and data array is not empty -> check for expired shots and delete them, then add new shot to the array

    if ((NULL == new_shot) && (NULL == *shot_data_array))
    {
        return SUCCESS;
    }

    uint8_t expired_shots = 0;
    if (NULL != *shot_data_array)
    {
        /// clearing shot_data_array of expired shots
        for (uint8_t i = 0; i < *ptr_shot_data_array_size; i += 1)
        {
            bool all_shot_indices_hit = true;
            if (NULL != (*shot_data_array)[i]) // gets pointer i in the pointer array
            {
                for (uint8_t j = 0; j < (*shot_data_array)[i]->array_size; j += 1)
                {
                    if (false == (*shot_data_array)[i]->shot_hit_smth_array[j])
                    {
                        all_shot_indices_hit = false;
                    }
                }
                if (true == all_shot_indices_hit)
                {
                    free((*shot_data_array)[i]->shot_hit_smth_array);
                    free((*shot_data_array)[i]->shot_index_array);
                    free((*shot_data_array)[i]);
                    (*shot_data_array)[i] = NULL;
                    expired_shots += 1;
                }
            }
        }
    }

    uint8_t new_shot_data_array_size = *ptr_shot_data_array_size - expired_shots;

    if (NULL != new_shot)
    {
        new_shot_data_array_size += 1;
    }

    /// if new_shot_data_array_size == *ptr_shot_data_array_size there is no need to create new array coz no shot expired and no new shot needs to be added
    if (new_shot_data_array_size != *ptr_shot_data_array_size)
    {
        shot_data_t **new_shot_data_array = NULL;
        if (new_shot_data_array_size > 0)
        {
            if (NULL == (new_shot_data_array = malloc(sizeof(**shot_data_array) * new_shot_data_array_size)))
            {
                ESP_LOGE(TAG, "Memory alloaction failed while creating:\nnew_shot_data_array\n");
            }
        }

        /// if shot_data_array is not empty, copy all non NULL values from the shot_data_array to the new_shot_data_array
        if ((NULL != *shot_data_array) && (NULL != new_shot_data_array))
        {
            uint8_t shots_copied_counter = 0;
            for (uint8_t i = 0; i < *ptr_shot_data_array_size; i += 1)
            {
                if (NULL != (*shot_data_array)[i])
                {
                    new_shot_data_array[shots_copied_counter] = (*shot_data_array)[i];
                    shots_copied_counter += 1;
                }
            }
        }

        if ((NULL != new_shot) && (NULL != new_shot_data_array))
        {
            new_shot_data_array[new_shot_data_array_size - 1] = new_shot;
        }

        *shot_data_array = new_shot_data_array;
        *ptr_shot_data_array_size = new_shot_data_array_size;
    }

    return SUCCESS;
}
