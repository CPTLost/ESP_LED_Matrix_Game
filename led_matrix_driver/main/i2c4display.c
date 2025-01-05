

#include "i2c4display.h"

#include "esp_log.h"

static const char *TAG = "I2C4DISPLAY";

esp_err_t initI2C(i2c_port_t i2c_num)
{
    ESP_LOGI(TAG, "init I2C");
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_5,
        .scl_io_num = GPIO_NUM_6,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_BITRATE};
    i2c_param_config(i2c_num, &conf);
    esp_err_t res = i2c_driver_install(i2c_num, conf.mode, 0, 0, 0);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_driver_install() FAILED: %d!", res);
    }
    else
    {
        ESP_LOGI(TAG, "i2c_driver_install() OK");
    }
    return res;
}