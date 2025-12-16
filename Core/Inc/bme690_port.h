#pragma once

#include "stm32wb0x_hal.h"
#include "bme69x.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initializes a bme69x_dev for I2C with a specific 7-bit address (0x76/0x77)
int8_t bme690_port_init_i2c(struct bme69x_dev *dev, I2C_HandleTypeDef *hi2c, uint8_t i2c_addr_7bit);

// Bosch SensorAPI callback signatures
BME69X_INTF_RET_TYPE bme690_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr);
BME69X_INTF_RET_TYPE bme690_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr);
void bme690_delay_us(uint32_t period, void *intf_ptr);

#ifdef __cplusplus
}
#endif
