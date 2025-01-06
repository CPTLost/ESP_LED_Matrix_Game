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
#include "driver/gptimer.h"

#include "led_strip.h"
#include "graphics.h"

#include "return_val.h"
#include "led_matrix_driver.h"
#include "asteroid_logic.h"
#include "game_logic.h"
#include "player_logic.h"
#include "shot_logic.h"
#include "i2c4display.h"

#define DOOMSDAY 0

#define PLAYER_UPDATE_SPEED_MS CONFIG_PLAYER_UPDATE_DELAY_IN_MS
#define FRAME_UPDATE_TIME_MS CONFIG_FRAME_UPDATE_DELAY_IN_MS
#define SHOT_COOLDOWN_IN_MS CONFIG_SHOT_COOLDOWN_IN_MS

#define SHOT_BTN_1_GPIO CONFIG_RIGHT_BTN_GPIO_PIN
#define SHOT_BTN_2_GPIO CONFIG_LEFT_BTN_GPIO_PIN

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
static TaskHandle_t gameDisplay_TaskHandler = NULL;

/// global variables
static led_matrix_data_t *g_matrix_data_buffer = NULL;
static led_matrix_data_t *g_asteroid_data_buffer = NULL;
static uint32_t g_asteroid_delay_ms = CONFIG_ASTEROID_UPDATE_DELAY_IN_MS;
static shot_data_t **g_shot_data_array = NULL; // pointer array
static uint8_t g_shot_data_array_size = 0;
static player_data_t *g_player_data_buffer = NULL;
// static uint64_t survival_time = 0;
static bool game_started = false;
static bool game_lost = false;
static bool stop_asteriods = true;

/// task/function declarations
void updateGame_Task(void *param);
void updateLedMatrix_Task(void *param);
void updateAsteroidField_Task(void *param);
void createShot_Task(void *param);
void updateShotDataArray_Task(void *param);
void updatePlayer_Task(void *param);
void gameDisplay_Task(void *param);

void isrCallbackFunction(void);

void app_main(void)
{
    initLedMatrix();
    init_buttons_for_shot_trigger(SHOT_BTN_1_GPIO, SHOT_BTN_2_GPIO);
    ESP_ERROR_CHECK(initI2C(I2C_INTERFACE));
    ESP_ERROR_CHECK(graphics_init(I2C_INTERFACE, CONFIG_GRAPHICS_PIXELWIDTH, CONFIG_GRAPHICS_PIXELHEIGHT, 0, true, false));

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
    xTaskCreate(gameDisplay_Task, "gameDisplay_Task", STD_TASK_STACKSIZE, NULL, STD_TASK_PRIORITY, &gameDisplay_TaskHandler);
}

void gameDisplay_Task(void *param)
{
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    while (!DOOMSDAY)
    {
        // Starting Screen
        graphics_startUpdate();
        graphics_println("         ASTEROID");
        graphics_println("       DESTROYER");
        graphics_finishUpdate();

        while (!game_started)
        {
            graphics_startUpdate();
            graphics_setCursor(0, 40);
            graphics_println("    Press Both Btns");
            graphics_finishUpdate();
            vTaskDelay(900 / portTICK_PERIOD_MS);
            graphics_startUpdate();
            graphics_clearRegion(0, 40, 128, 64);
            graphics_finishUpdate();
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        stop_asteriods = false;
        ESP_ERROR_CHECK(gptimer_start(gptimer));

        char time[25];
        while (!game_lost)
        {
            uint64_t survival_time_in_us = 0;
            ESP_ERROR_CHECK(gptimer_get_raw_count(gptimer, &survival_time_in_us));
            uint32_t minutes = (survival_time_in_us / 1000000) / 60;
            uint32_t seconds = (survival_time_in_us / 1000000) % 60;
            uint32_t milliseconds = (survival_time_in_us / 1000) % 60;
            snprintf(time, sizeof(time), "Survived: %ld:%ld:%ld", minutes, seconds, milliseconds);

            graphics_startUpdate();
            graphics_clearRegion(0, 40, 128, 64);
            graphics_setCursor(0, 40);
            graphics_println(time);
            graphics_finishUpdate();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        game_started = false;
        ESP_ERROR_CHECK(gptimer_stop(gptimer));
        ESP_ERROR_CHECK(gptimer_set_raw_count(gptimer, 0));
        graphics_startUpdate();
        graphics_clearRegion(0, 40, 128, 64);
        graphics_setCursor(0, 40);
        graphics_println(time);
        graphics_finishUpdate();

        while (game_lost)
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        stop_asteriods = true;
        xSemaphoreTake(gMutex_asteroid_data_buffer, portMAX_DELAY);
        if (NULL != g_asteroid_data_buffer)
        {
            free(g_asteroid_data_buffer->ptr_index_array_leds_to_set);
            free(g_asteroid_data_buffer->ptr_rgb_array_leds_to_set);
            free(g_asteroid_data_buffer);
        }
        xSemaphoreGive(gMutex_asteroid_data_buffer);
    }
}

void updateGame_Task(void *param)
{
    while (!DOOMSDAY)
    {
        /// changing asteroid delay still missing and maybe also changing row_offset
        uint32_t notification_value = 0;
        xTaskNotifyWait(0x00, UINT32_MAX, &notification_value, 0);
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
                g_player_data_buffer, g_shot_data_array, g_shot_data_array_size, &game_lost);

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

        if (!stop_asteriods)
        {
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
        if (game_started)
        {
            xSemaphoreTake(gMutex_player_data_buffer, portMAX_DELAY);

            updatePlayerPosition(g_player_data_buffer, (gpio_get_level(SHOT_BTN_2_GPIO) == 0), (gpio_get_level(SHOT_BTN_1_GPIO) == 0));

            xSemaphoreGive(gMutex_player_data_buffer);
        }
        vTaskDelay(PLAYER_UPDATE_SPEED_MS / portTICK_PERIOD_MS);
    }
}

void isrCallbackFunction(void)
{
    if (!game_started && !game_lost)
    {
        game_started = true;
    }
    else if (!game_started && game_lost)
    {
        game_lost = false;
    }
    else
    {
        xTaskNotifyFromISR(createShot_TaskHandler, 0, eNoAction, 0);
    }
}