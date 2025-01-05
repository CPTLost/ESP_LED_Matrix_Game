#include <stdio.h>
#include <inttypes.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <limits.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_random.h>
#include <driver/gpio.h>

#include "led_strip.h"

#include "return_val.h"
#include "led_matrix_driver.h"
#include "asteroid_logic.h"
#include "game_logic.h"
#include "player_logic.h"
#include "shot_logic.h"

#define DOOMSDAY 0

#define PLAYER_UPDATE_SPEED_MS 100
#define FRAME_UPDATE_TIME_MS 10
#define SHOT_COOLDOWN_IN_MS 200

#define SHOT_BTN_1_GPIO 2 // right button
#define SHOT_BTN_2_GPIO 9 // left button

#define STD_TASK_STACKSIZE 4096
#define STD_TASK_PRIORITY 3

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

static const char *TAG_MAIN = "MAIN_GAME";

/// Mutexes
static SemaphoreHandle_t gMutex_matrix_data_buffer;
static SemaphoreHandle_t gMutex_asteroid_data_buffer;
static SemaphoreHandle_t gMutex_shot_data_array;
static SemaphoreHandle_t gMutex_player_data_buffer;
static SemaphoreHandle_t gMutex_asteroid_delay_ms;
static SemaphoreHandle_t gMutex_shot_data_array_size;

/// Task handlers
static TaskHandle_t updateGame_TaskHandler = NULL;
static TaskHandle_t updateAsteroidField_TaskHandler = NULL;
static TaskHandle_t updateLedMatrix_TaskHandler = NULL;
static TaskHandle_t createShot_TaskHandler = NULL;
static TaskHandle_t updateShotDataArray_TaskHandler = NULL;
static TaskHandle_t updatePlayer_TaskHandler = NULL;

static TaskHandle_t test_create_shot_TaskHandler = NULL;

/// global variables
static led_matrix_data_t *g_matrix_data_buffer = NULL;
static led_matrix_data_t *g_asteroid_data_buffer = NULL;
static uint32_t g_asteroid_delay_ms = 300;
static shot_data_t **g_shot_data_array = NULL; // pointer array
static uint8_t g_shot_data_array_size = 0;
static player_data_t *g_player_data_buffer = NULL;

// static bool new_asteroid_data_flag = false;

/// task declarations
void updateGame_Task(void *param);
void updateLedMatrix_Task(void *param);
void updateAsteroidField_Task(void *param);
void createShot_Task(void *param);
void updateShotDataArray_Task(void *param);
// still needs to be implemented
void updatePlayer_Task(void *param);

void trigger_shot(void)
{
    xTaskNotifyFromISR(createShot_TaskHandler, 0, eNoAction, 0);
}

void app_main(void)
{

    ////////////// temp test code :
    // static uint16_t player_index_array[] = {7, 8, 9, 8 + MATRIX_SIDE_LENGTH};
    // static uint16_t player_shot_start_pos = 8;
    // static player_data_t normal_player_data = {.player_type = &normal_player,
    //                                            .shot_start_position = 8,
    //                                            .player_index_array = player_index_array,
    //                                            .array_size = ARRAY_LENGTH(player_index_array)};

    // g_player_data_buffer = &normal_player_data;
    /////////

    // config interrupts etc

    initLedMatrix();
    // init_button_for_shot_trigger(SHOT_BTN_GPIO);

    init_buttons_for_shot_trigger(SHOT_BTN_1_GPIO, SHOT_BTN_2_GPIO);

    g_player_data_buffer = initPlayer(&normal_player);

    gMutex_matrix_data_buffer = xSemaphoreCreateMutex();
    gMutex_asteroid_data_buffer = xSemaphoreCreateMutex();
    gMutex_shot_data_array = xSemaphoreCreateMutex();
    gMutex_player_data_buffer = xSemaphoreCreateMutex();
    gMutex_asteroid_delay_ms = xSemaphoreCreateMutex();
    gMutex_shot_data_array_size = xSemaphoreCreateMutex();

    xTaskCreate(updateGame_Task, "updateGame_Task", STD_TASK_STACKSIZE, NULL, STD_TASK_PRIORITY, &updateGame_TaskHandler);
    xTaskCreate(updateLedMatrix_Task, "updateLedMatrix_Task", STD_TASK_STACKSIZE, NULL, STD_TASK_PRIORITY, &updateLedMatrix_TaskHandler);
    xTaskCreate(updateAsteroidField_Task, "updateAsteroidField_Task", STD_TASK_STACKSIZE, NULL, STD_TASK_PRIORITY, &updateAsteroidField_TaskHandler);
    xTaskCreate(createShot_Task, "createShot_Task", STD_TASK_STACKSIZE, NULL, STD_TASK_PRIORITY, &createShot_TaskHandler);
    xTaskCreate(updateShotDataArray_Task, "updateShotDataArray_Task", STD_TASK_STACKSIZE, NULL, STD_TASK_PRIORITY, &updateShotDataArray_TaskHandler);
    xTaskCreate(updatePlayer_Task, "updatePlayer_Task", STD_TASK_STACKSIZE, NULL, STD_TASK_PRIORITY, &updatePlayer_TaskHandler);
}

void updateGame_Task(void *param)
{
    while (!DOOMSDAY)
    {
        /// changing asteroid delay still missing and maybe also changing row_offset
        uint32_t notification_value = 0;
        xTaskNotifyWait(0x00, UINT32_MAX, &notification_value, 0);
        // xTaskNotifyWait(0x00, UINT32_MAX, &notification_value, portMAX_DELAY);
        if (NULL != g_asteroid_data_buffer)
        {

            xSemaphoreTake(gMutex_asteroid_data_buffer, portMAX_DELAY);
            xSemaphoreTake(gMutex_player_data_buffer, portMAX_DELAY);
            xSemaphoreTake(gMutex_shot_data_array, portMAX_DELAY);
            xSemaphoreTake(gMutex_shot_data_array_size, portMAX_DELAY);

            bool new_asteroid_data_flag = false;
            if ((notification_value & 0x01) != 0)
            {
                new_asteroid_data_flag = true;
            }

            led_matrix_data_t *new_matrix_data = updateGame(
                g_asteroid_data_buffer, new_asteroid_data_flag,
                g_player_data_buffer, g_shot_data_array, g_shot_data_array_size);

            xSemaphoreGive(gMutex_asteroid_data_buffer);
            xSemaphoreGive(gMutex_player_data_buffer);
            xSemaphoreGive(gMutex_shot_data_array);
            xSemaphoreGive(gMutex_shot_data_array_size);

            xSemaphoreTake(gMutex_matrix_data_buffer, portMAX_DELAY);
            if (NULL != g_matrix_data_buffer)
            {
                free(g_matrix_data_buffer->ptr_index_array_leds_to_set);
                free(g_matrix_data_buffer->ptr_rgb_array_leds_to_set);
                free(g_matrix_data_buffer);
            }
            g_matrix_data_buffer = new_matrix_data;
            xSemaphoreGive(gMutex_matrix_data_buffer);

            xTaskNotify(updateLedMatrix_TaskHandler, 0, eNoAction);
            xTaskNotify(updateShotDataArray_TaskHandler, 0, eNoAction);
        }

        vTaskDelay(FRAME_UPDATE_TIME_MS / portTICK_PERIOD_MS);
    }
}

void updateLedMatrix_Task(void *param)
{
    while (!DOOMSDAY)
    {
        xTaskNotifyWait(0x00, UINT32_MAX, NULL, portMAX_DELAY);
        xSemaphoreTake(gMutex_matrix_data_buffer, portMAX_DELAY);

        updateLedMatrix(g_matrix_data_buffer);

        xSemaphoreGive(gMutex_matrix_data_buffer);
    }
}

void updateAsteroidField_Task(void *param)
{
    while (!DOOMSDAY)
    {
        xSemaphoreTake(gMutex_asteroid_delay_ms, portMAX_DELAY);
        uint32_t delay = g_asteroid_delay_ms;
        xSemaphoreGive(gMutex_asteroid_delay_ms);

        vTaskDelay(delay / portTICK_PERIOD_MS);

        led_matrix_data_t *new_asteroid_data = updateAsteroidField();

        xSemaphoreTake(gMutex_asteroid_data_buffer, portMAX_DELAY);
        if (NULL != g_asteroid_data_buffer)
        {
            free(g_asteroid_data_buffer->ptr_index_array_leds_to_set);
            free(g_asteroid_data_buffer->ptr_rgb_array_leds_to_set);
            free(g_asteroid_data_buffer);
        }
        g_asteroid_data_buffer = new_asteroid_data;
        xSemaphoreGive(gMutex_asteroid_data_buffer);
        xTaskNotify(updateGame_TaskHandler, 0x1, eSetBits);
    }
}

void createShot_Task(void *param)
{
    while (!DOOMSDAY)
    {
        // should be executed when button interrupt is triggered but with a max shot rate
        // so maybe task wait on itr

        // maybe start timer when shot is created and only create new shot when a cretain time has passed
        xTaskNotifyWait(0x00, UINT32_MAX, NULL, portMAX_DELAY);

        xSemaphoreTake(gMutex_player_data_buffer, portMAX_DELAY);
        shot_data_t *new_shot = createShot(g_player_data_buffer, &normal_shot_type);
        xSemaphoreGive(gMutex_player_data_buffer);

        xSemaphoreTake(gMutex_shot_data_array_size, portMAX_DELAY);
        xSemaphoreTake(gMutex_shot_data_array, portMAX_DELAY);

        updatedShotDataArray(new_shot, &g_shot_data_array, &g_shot_data_array_size);

        xSemaphoreGive(gMutex_shot_data_array_size);
        xSemaphoreGive(gMutex_shot_data_array);

        xTaskNotify(updateGame_TaskHandler, NULL, eNoAction); // do I still need this?
        vTaskDelay(SHOT_COOLDOWN_IN_MS / portTICK_PERIOD_MS);
    }
}

void updateShotDataArray_Task(void *param)
{
    while (!DOOMSDAY)
    {
        xTaskNotifyWait(0x00, UINT32_MAX, NULL, portMAX_DELAY);

        xSemaphoreTake(gMutex_shot_data_array_size, portMAX_DELAY);
        xSemaphoreTake(gMutex_shot_data_array, portMAX_DELAY);

        updatedShotDataArray(NULL, &g_shot_data_array, &g_shot_data_array_size);

        xSemaphoreGive(gMutex_shot_data_array_size);
        xSemaphoreGive(gMutex_shot_data_array);
    }
}

void updatePlayer_Task(void *param)
{
    while (!DOOMSDAY)
    {
        xSemaphoreTake(gMutex_player_data_buffer, portMAX_DELAY);

        updatePlayerPosition(g_player_data_buffer, (gpio_get_level(SHOT_BTN_2_GPIO) == 0), (gpio_get_level(SHOT_BTN_1_GPIO) == 0));

        xSemaphoreGive(gMutex_player_data_buffer);

        vTaskDelay(PLAYER_UPDATE_SPEED_MS / portTICK_PERIOD_MS);
    }
}
