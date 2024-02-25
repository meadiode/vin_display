
#include <string.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <pico/time.h>

#include "hdsp2112.h"

#define HDSP_NUM_DISPLAYS    6
#define HDSP_NUM_CHARS       HDSP_NUM_DISPLAYS * 8

#define HDSP_DATA_START_PIN  6
#define HDSP_DATA_PINS_NUM   7

#define HDSP_ADDR_START_PIN  14
#define HDSP_ADDR_PINS_NUM   3

#define HDSP_CADDR_START_PIN 17
#define HDSP_CADDR_PINS_NUM  2

#define HDSP_CE_START_PIN    20
#define HDSP_CE_PINS_NUM     3

#define HDSP_RST_PIN         2
#define HDSP_FL_PIN          3
#define HDSP_WR_PIN          19
#define HDSP_RD_PIN          23

#define HDSP_DATA_PINS_MASK  (((1 << HDSP_DATA_PINS_NUM) - 1) << HDSP_DATA_START_PIN)
#define HDSP_ADDR_PINS_MASK  (((1 << HDSP_ADDR_PINS_NUM) - 1) << HDSP_ADDR_START_PIN)
#define HDSP_CADDR_PINS_MASK (((1 << HDSP_CADDR_PINS_NUM) - 1) << HDSP_CADDR_START_PIN)
#define HDSP_CE_PINS_MASK    (((1 << HDSP_CE_PINS_NUM) - 1) << HDSP_CE_START_PIN)

#define HDSP_PIO pio0


#include "hdsp2112.pio.h"

static uint8_t display_buf[8 * HDSP_NUM_DISPLAYS] = {0};
static int32_t hdsp_dma_chan = -1; 


void hdsp2112_init(void)
{
    gpio_init(HDSP_WR_PIN);
    gpio_init(HDSP_RD_PIN);
    gpio_init(HDSP_RST_PIN);
    gpio_init(HDSP_FL_PIN);

    gpio_init_mask(HDSP_DATA_PINS_MASK);
    gpio_init_mask(HDSP_ADDR_PINS_MASK);
    gpio_init_mask(HDSP_CADDR_PINS_MASK);
    gpio_init_mask(HDSP_CE_PINS_MASK);

    gpio_set_dir(HDSP_WR_PIN, true);
    gpio_set_dir(HDSP_RD_PIN, true);
    gpio_set_dir(HDSP_RST_PIN, true);
    gpio_set_dir(HDSP_FL_PIN, true);

    gpio_set_dir_out_masked(HDSP_DATA_PINS_MASK);
    gpio_set_dir_out_masked(HDSP_ADDR_PINS_MASK);
    gpio_set_dir_out_masked(HDSP_CADDR_PINS_MASK);
    gpio_set_dir_out_masked(HDSP_CE_PINS_MASK);
 
    gpio_put(HDSP_RST_PIN, 1);
    gpio_put(HDSP_FL_PIN, 1);
    gpio_put(HDSP_WR_PIN, 1);
    gpio_put(HDSP_RD_PIN, 1);
    gpio_put(HDSP_CADDR_START_PIN + 0, 1);
    gpio_put(HDSP_CADDR_START_PIN + 1, 1);

    gpio_put(HDSP_RST_PIN, 0);
    sleep_us(100u);
    gpio_put(HDSP_RST_PIN, 1);

    // hdsp2112_bitbang_test();

    hdsp2112_program_init(HDSP_PIO);

    hdsp_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(hdsp_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_PIO0_TX0);

    dma_channel_configure(
        hdsp_dma_chan,
        &cfg,
        &pio0_hw->txf[HDSP2112_PIO_DISPLAY_CHAR_SM], /* Write address */
        display_buf,                                 /* Read address */
        sizeof(display_buf),
        false                                        /* Don't start yet */
    );

    hdsp2112_print("111111112222222233333333444444445555555566666666");

    for (uint8_t i = 0; i < 3; i++)
    {
        hdsp2112_set_brightness(255);
        sleep_ms(300);
        hdsp2112_set_brightness(33);
        sleep_ms(300);
    }

    hdsp2112_print("");
}


void hdsp2112_print(const uint8_t *str)
{
    hdsp2112_print_buf(str, strnlen(str, sizeof(display_buf)));
}


void hdsp2112_print_buf(const uint8_t *buf, size_t buflen)
{
    dma_channel_wait_for_finish_blocking(hdsp_dma_chan);
    sleep_us(10);
    
    gpio_put(HDSP_CADDR_START_PIN + 0, 1);
    gpio_put(HDSP_CADDR_START_PIN + 1, 1);

    memset(display_buf, ' ', sizeof(display_buf));
    memcpy(display_buf, buf, MIN(buflen, sizeof(display_buf)));

    dma_channel_set_read_addr(hdsp_dma_chan, display_buf, true);    
}


void hdsp2112_set_brightness(uint8_t brightness)
{
    static uint8_t ctrl_buf[8 * HDSP_NUM_DISPLAYS] = {0};
    uint8_t cw = (0xff - brightness) / 32;

    dma_channel_wait_for_finish_blocking(hdsp_dma_chan);
    sleep_us(10);

    gpio_put(HDSP_CADDR_START_PIN + 0, 0);
    gpio_put(HDSP_CADDR_START_PIN + 1, 1);

    memset(ctrl_buf, cw, sizeof(ctrl_buf));
    dma_channel_set_read_addr(hdsp_dma_chan, ctrl_buf, true);

    dma_channel_wait_for_finish_blocking(hdsp_dma_chan);
}

void hdsp2112_bitbang_test(void)
{
    uint8_t text[] = "***<1>*****<2>*****<3>*****<4>*****<5>*****<6>**";

    for (;;)
    {
        for (uint8_t i = 0; i < HDSP_NUM_CHARS; i++)
        {
            uint8_t disp_id = i / 8;
            uint8_t char_id = i % 8;

            uint8_t c = text[i];

            gpio_put(HDSP_DATA_START_PIN + 0, (c >> 0) & 0x01);
            gpio_put(HDSP_DATA_START_PIN + 1, (c >> 1) & 0x01);
            gpio_put(HDSP_DATA_START_PIN + 2, (c >> 2) & 0x01);
            gpio_put(HDSP_DATA_START_PIN + 3, (c >> 3) & 0x01);
            gpio_put(HDSP_DATA_START_PIN + 4, (c >> 4) & 0x01);
            gpio_put(HDSP_DATA_START_PIN + 5, (c >> 5) & 0x01);
            gpio_put(HDSP_DATA_START_PIN + 6, (c >> 6) & 0x01);
            gpio_put(HDSP_DATA_START_PIN + 7, 0);

            gpio_put(HDSP_ADDR_START_PIN + 0, (char_id >> 0) & 0x01);
            gpio_put(HDSP_ADDR_START_PIN + 1, (char_id >> 1) & 0x01);
            gpio_put(HDSP_ADDR_START_PIN + 2, (char_id >> 2) & 0x01);
            gpio_put(HDSP_CADDR_START_PIN + 0, 1);
            gpio_put(HDSP_CADDR_START_PIN + 1, 1);

            gpio_put(HDSP_CE_START_PIN + 0, (disp_id >> 0) & 0x01);
            gpio_put(HDSP_CE_START_PIN + 1, (disp_id >> 1) & 0x01);
            gpio_put(HDSP_CE_START_PIN + 2, (disp_id >> 2) & 0x01);
            sleep_ms(1u);

            gpio_put(HDSP_WR_PIN, 0);
            sleep_ms(1u);
            gpio_put(HDSP_WR_PIN, 1);
            sleep_ms(1u);

            gpio_put(HDSP_CE_START_PIN + 0, 1);
            gpio_put(HDSP_CE_START_PIN + 1, 1);
            gpio_put(HDSP_CE_START_PIN + 2, 1);
        }

        sleep_ms(3000u);

        uint8_t lastc = text[47];
        memmove(text + 1, text, 47);
        text[0] = lastc;
    }

}
