
#include <string.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>

#include "hdsp2112.h"
#include "hdsp2112.pio.h"


#define HDSP_NUM_DISPLAYS 3

#define HDSP_DATA_START_PIN 10
#define HDSP_ADDR_START_PIN 2
#define HDSP_CE_START_PIN   7

#define HDSP_CE_DISP1       7
#define HDSP_CE_DISP2       8
#define HDSP_CE_DISP3       9

#define HDSP_RST_PIN        17
#define HDSP_FL_PIN         18
#define HDSP_WR_PIN         19


static uint8_t display_buf[8 * HDSP_NUM_DISPLAYS] = {0};
static int32_t hdsp_dma_chan = -1; 


void hdsp2112_init(void)
{

    // uint32_t mask = 0;
    // mask |= (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
    // mask |= (1 << 10) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 14) | (1 << 15) | (1 << 16) | (1 << 17);
    // mask |= (1 << 7) | (1 << 8) | (1 << 9);
    // mask |= (1 << 17) | (1 << 18) | (1 << 19);

    // gpio_init_mask(mask);
    // gpio_set_dir_out_masked(mask);

    // gpio_put(HDSP_CE_DISP1, 1);
    // gpio_put(HDSP_CE_DISP2, 1);
    // gpio_put(HDSP_CE_DISP3, 1);

    // gpio_put(HDSP_RST_PIN, 1);
    // gpio_put(HDSP_FL_PIN, 1);
    // gpio_put(HDSP_WR_PIN, 1);

    // gpio_put(HDSP_ADDR_START_PIN + 3, 1);
    // gpio_put(HDSP_ADDR_START_PIN + 4, 1);

    // uint8_t c = 'A';
    // const uint8_t *str = ">DESTRUCTION IN 00m 49s<";

    // for (uint8_t d = 0; d < 3; d++)
    // {
    //     gpio_put(HDSP_CE_DISP1, d == 0 ? 0 : 1);
    //     gpio_put(HDSP_CE_DISP2, d == 1 ? 0 : 1);
    //     gpio_put(HDSP_CE_DISP3, d == 2 ? 0 : 1);

    //     for (uint32_t i = 0; i < 8; i++)
    //     {
    //         gpio_put(HDSP_ADDR_START_PIN + 0, (i >> 0) & 0x01);
    //         gpio_put(HDSP_ADDR_START_PIN + 1, (i >> 1) & 0x01);
    //         gpio_put(HDSP_ADDR_START_PIN + 2, (i >> 2) & 0x01);

    //         gpio_put(HDSP_DATA_START_PIN + 0, (str[d * 8 + i] >> 0) & 0x01);
    //         gpio_put(HDSP_DATA_START_PIN + 1, (str[d * 8 + i] >> 1) & 0x01);
    //         gpio_put(HDSP_DATA_START_PIN + 2, (str[d * 8 + i] >> 2) & 0x01);
    //         gpio_put(HDSP_DATA_START_PIN + 3, (str[d * 8 + i] >> 3) & 0x01);
    //         gpio_put(HDSP_DATA_START_PIN + 4, (str[d * 8 + i] >> 4) & 0x01);
    //         gpio_put(HDSP_DATA_START_PIN + 5, (str[d * 8 + i] >> 5) & 0x01);
    //         gpio_put(HDSP_DATA_START_PIN + 6, (str[d * 8 + i] >> 6) & 0x01);

    //         gpio_put(HDSP_WR_PIN, 0);
    //         sleep_ms(1);
    //         gpio_put(HDSP_WR_PIN, 1);
    //         sleep_ms(1);

    //         sleep_ms(100);
    //     }
    //     gpio_put(HDSP_CE_DISP1, 1);
    //     gpio_put(HDSP_CE_DISP2, 1);
    //     gpio_put(HDSP_CE_DISP3, 1);
    // }





    gpio_init(HDSP_RST_PIN);
    gpio_init(HDSP_FL_PIN);
    gpio_init(HDSP_ADDR_START_PIN + 3);
    gpio_init(HDSP_ADDR_START_PIN + 4);

    gpio_set_dir(HDSP_RST_PIN, true);
    gpio_set_dir(HDSP_FL_PIN, true);
    gpio_set_dir(HDSP_ADDR_START_PIN + 3, true);
    gpio_set_dir(HDSP_ADDR_START_PIN + 4, true);

    gpio_put(HDSP_RST_PIN, 0);
    sleep_ms(10);
    gpio_put(HDSP_RST_PIN, 1);

    gpio_put(HDSP_FL_PIN, 1);
    gpio_put(HDSP_ADDR_START_PIN + 3, 1);
    gpio_put(HDSP_ADDR_START_PIN + 4, 1);



    gpio_init(HDSP_ADDR_START_PIN + 0);
    gpio_init(HDSP_ADDR_START_PIN + 1);
    gpio_init(HDSP_ADDR_START_PIN + 2);
    gpio_set_dir(HDSP_ADDR_START_PIN + 0, true);
    gpio_set_dir(HDSP_ADDR_START_PIN + 1, true);
    gpio_set_dir(HDSP_ADDR_START_PIN + 2, true);

    gpio_put(HDSP_ADDR_START_PIN + 0, 0);
    gpio_put(HDSP_ADDR_START_PIN + 1, 0);
    gpio_put(HDSP_ADDR_START_PIN + 2, 0);



    hdsp2112_program_init(pio0, HDSP_DATA_START_PIN, HDSP_ADDR_START_PIN,
                          HDSP_CE_START_PIN, HDSP_WR_PIN);

    

    hdsp_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(hdsp_dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        hdsp_dma_chan,
        &c,
        &pio0_hw->txf[HDSP2112_PIO_DISPLAY_CHAR_SM], /* Write address */
        display_buf,                                 /* Read address */
        sizeof(display_buf),
        false                                        /* Don't start yet */
    );


    for (uint8_t i = 0; i < 100; i++)
    {
        hdsp2112_print("     <<< SUPER >>>      ");
        sleep_ms(1000);
        hdsp2112_print("      <<< HOT >>>       ");
        sleep_ms(1000);
    }
   
}

void hdsp2112_print(const uint8_t *str)
{

    memset(display_buf, ' ', sizeof(display_buf));
    memcpy(display_buf, str, strnlen(str, sizeof(display_buf)));

    pio_sm_put(pio0, HDSP2112_PIO_SELECT_DISPLAY_SM, 0b110); 
    pio_sm_put(pio0, HDSP2112_PIO_SELECT_DISPLAY_SM, 0b101); 
    pio_sm_put(pio0, HDSP2112_PIO_SELECT_DISPLAY_SM, 0b011); 

    dma_channel_set_read_addr(hdsp_dma_chan, display_buf, true);
}