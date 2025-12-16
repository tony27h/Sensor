#include "bme690_port.h"

// We store both I2C handle + addr in one struct and pass as intf_ptr
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint16_t dev_addr_8bit; // HAL expects 8-bit address (7-bit << 1)
} bme690_i2c_ctx_t;

static bme690_i2c_ctx_t s_ctx[2]; // two sensors
static uint8_t s_ctx_count = 0;

int8_t bme690_port_init_i2c(struct bme69x_dev *dev, I2C_HandleTypeDef *hi2c, uint8_t i2c_addr_7bit)
{
    if (!dev || !hi2c) return BME69X_E_NULL_PTR;
    if (s_ctx_count >= 2) return BME69X_E_INVALID_LENGTH;

    bme690_i2c_ctx_t *ctx = &s_ctx[s_ctx_count++];
    ctx->hi2c = hi2c;
    ctx->dev_addr_8bit = (uint16_t)(i2c_addr_7bit << 1);

    dev->intf = BME69X_I2C_INTF;
    dev->read = bme690_i2c_read;
    dev->write = bme690_i2c_write;
    dev->delay_us = bme690_delay_us;
    dev->intf_ptr = ctx;

    dev->amb_temp = 25;     // recommended default, update if you know ambient
    dev->mem_page = 0; // not used for I2C but keep initialized
    dev->info_msg = 0;

    return BME69X_OK;
}

BME69X_INTF_RET_TYPE bme690_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    bme690_i2c_ctx_t *ctx = (bme690_i2c_ctx_t *)intf_ptr;
    if (!ctx || !ctx->hi2c || !reg_data) return -1;

    if (HAL_I2C_Mem_Read(ctx->hi2c,
                         ctx->dev_addr_8bit,
                         reg_addr,
                         I2C_MEMADD_SIZE_8BIT,
                         reg_data,
                         (uint16_t)length,
                         100) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

BME69X_INTF_RET_TYPE bme690_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    bme690_i2c_ctx_t *ctx = (bme690_i2c_ctx_t *)intf_ptr;
    if (!ctx || !ctx->hi2c || !reg_data) return -1;

    if (HAL_I2C_Mem_Write(ctx->hi2c,
                          ctx->dev_addr_8bit,
                          reg_addr,
                          I2C_MEMADD_SIZE_8BIT,
                          (uint8_t*)reg_data,
                          (uint16_t)length,
                          100) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

void bme690_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    // crude microsecond delay using HAL_GetTick (ms resolution)
    // Good enough for many apps; for accurate us use a hardware timer.
    uint32_t ms = (period + 999) / 1000;
    HAL_Delay(ms);
}
