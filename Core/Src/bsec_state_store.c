#include "bsec_state_store.h"

#include "bsec_interface.h"
#include "bsec_datatypes.h"

#include "stm32wb0x_hal_flash.h"
#include "stm32wb0x_hal_flash_ex.h"

#include <string.h>

#ifndef BSEC_FLASH_PAGE_SIZE
#define BSEC_FLASH_PAGE_SIZE (FLASH_PAGE_SIZE) /* 2KB */
#endif

/* Store in last app page before BT NVM (BT NVM is last 4KB: 0x1006F000..0x1006FFFF) */
#ifndef BSEC_STORE_ADDR
#define BSEC_STORE_ADDR (0x1006E800u)
#endif

#define BSEC_STATE_MAGIC   (0x42534543u) /* 'BSEC' */
#define BSEC_STATE_VERSION (1u)

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    uint16_t length;
    uint32_t crc32;
    uint32_t save_count;
    uint8_t  reserved[12];
} bsec_state_hdr_t;

static uint32_t s_last_save_ms = 0;
static uint32_t s_save_count_cache = 0;

static uint32_t crc32_sw(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint32_t b = 0; b < 8; b++)
        {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static const bsec_state_hdr_t* hdr_ptr(void)
{
    return (const bsec_state_hdr_t*)BSEC_STORE_ADDR;
}

static const uint8_t* blob_ptr(void)
{
    return (const uint8_t*)(BSEC_STORE_ADDR + sizeof(bsec_state_hdr_t));
}

HAL_StatusTypeDef bsec_state_store_init(void)
{
    if ((BSEC_STORE_ADDR % BSEC_FLASH_PAGE_SIZE) != 0u) return HAL_ERROR;
    if (sizeof(bsec_state_hdr_t) >= BSEC_FLASH_PAGE_SIZE) return HAL_ERROR;
    return HAL_OK;
}

static HAL_StatusTypeDef flash_erase_one_page(uint32_t page_start_addr)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0;

    const uint32_t flash_base = 0x10040000u;
    uint32_t page = (page_start_addr - flash_base) / BSEC_FLASH_PAGE_SIZE;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Page = page;
    erase.NbPages = 1;

    return HAL_FLASHEx_Erase(&erase, &page_error);
}

static HAL_StatusTypeDef flash_program_words(uint32_t dst_addr, const uint8_t *src, uint32_t len)
{
    /* WB0x HAL programs 32-bit words */
    for (uint32_t i = 0; i < len; i += 4)
    {
        uint32_t w = 0xFFFFFFFFu;
        uint32_t chunk = (len - i) >= 4 ? 4 : (len - i);
        memcpy(&w, &src[i], chunk);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dst_addr + i, w) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef bsec_state_store_load(void *bsec_inst)
{
    if (!bsec_inst) return HAL_ERROR;

    const bsec_state_hdr_t *hdr = hdr_ptr();

    if (hdr->magic != BSEC_STATE_MAGIC) return HAL_ERROR;
    if (hdr->version != BSEC_STATE_VERSION) return HAL_ERROR;
    if (hdr->length == 0u || hdr->length > BSEC_MAX_STATE_BLOB_SIZE) return HAL_ERROR;

    const uint8_t *blob = blob_ptr();
    if (crc32_sw(blob, hdr->length) != hdr->crc32) return HAL_ERROR;

    static uint8_t workbuf[BSEC_MAX_WORKBUFFER_SIZE];
    bsec_library_return_t br = bsec_set_state(bsec_inst, blob, hdr->length, workbuf, sizeof(workbuf));
    if (br != BSEC_OK) return HAL_ERROR;

    s_save_count_cache = hdr->save_count;
    return HAL_OK;
}

HAL_StatusTypeDef bsec_state_store_maybe_save(void *bsec_inst,
                                              uint32_t now_ms,
                                              uint32_t save_period_ms)
{
    if (!bsec_inst) return HAL_ERROR;

    if (s_last_save_ms == 0u)
    {
        /* First call: start the timer, but do not save yet */
        s_last_save_ms = now_ms;
        return HAL_BUSY;
    }

    if ((now_ms - s_last_save_ms) < save_period_ms)
    {
        /* Not time yet; no save performed */
        return HAL_BUSY;
    }

    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_state = 0;
    static uint8_t workbuf[BSEC_MAX_WORKBUFFER_SIZE];

    bsec_library_return_t br = bsec_get_state(bsec_inst,
                                              0,
                                              state,
                                              sizeof(state),
                                              workbuf,
                                              sizeof(workbuf),
                                              &n_state);
    if (br != BSEC_OK) return HAL_ERROR;
    if (n_state == 0u || n_state > BSEC_MAX_STATE_BLOB_SIZE) return HAL_ERROR;

    bsec_state_hdr_t hdr = {0};
    hdr.magic = BSEC_STATE_MAGIC;
    hdr.version = BSEC_STATE_VERSION;
    hdr.length = (uint16_t)n_state;
    hdr.crc32 = crc32_sw(state, n_state);
    hdr.save_count = ++s_save_count_cache;

    uint8_t page[BSEC_FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));
    memcpy(page, &hdr, sizeof(hdr));
    memcpy(page + sizeof(hdr), state, n_state);

    if (flash_erase_one_page(BSEC_STORE_ADDR) != HAL_OK) return HAL_ERROR;
    if (flash_program_words(BSEC_STORE_ADDR, page, sizeof(page)) != HAL_OK) return HAL_ERROR;

    /* Save completed */
    s_last_save_ms = now_ms;
    return HAL_OK;
}
