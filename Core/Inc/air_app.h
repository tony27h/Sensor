#pragma once

#include "stm32wb0x_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float t_c;
    float rh;
    float p_pa;
    float iaq;
    uint8_t iaq_accuracy; // 0..3
} air_readings_t;

HAL_StatusTypeDef air_app_init(I2C_HandleTypeDef *hi2c1, UART_HandleTypeDef *huart1);
HAL_StatusTypeDef air_app_process(void);          // call frequently (e.g. in while(1))
HAL_StatusTypeDef air_app_get(air_readings_t *out); // latest readings

#ifdef __cplusplus
}
#endif
