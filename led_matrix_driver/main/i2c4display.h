

#ifndef I2C4DISPLAY
#define I2C4DISPLAY

#include "driver/i2c.h"

#define I2C_INTERFACE I2C_NUM_0
#define I2C_MASTER_BITRATE 400000

esp_err_t initI2C(i2c_port_t i2c_num);

#endif