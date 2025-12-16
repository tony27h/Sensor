#pragma once

#include "stm32wb0x_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef bsec_state_store_init(void);

/* Call after bsec_init() and bsec_set_configuration() */
HAL_StatusTypeDef bsec_state_store_load(void *bsec_inst);

/**
 * Call often; it saves at most once per interval.
 *
 * Return codes:
 *  - HAL_OK   : a save was performed successfully (flash updated)
 *  - HAL_BUSY : not time to save yet (no write done)
 *  - HAL_ERROR: save attempted but failed
 */
HAL_StatusTypeDef bsec_state_store_maybe_save(void *bsec_inst,
                                              uint32_t now_ms,
                                              uint32_t save_period_ms);

#ifdef __cplusplus
}
#endif
