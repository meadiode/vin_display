
#include <string.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <pico/time.h>

#include "mdl2416c.h"
#include "mdl2416c.pio.h"


#define MDL_NUM_DISPLAYS       10

#define MDL_DATA_START_PIN     6
#define MDL_ADDR_START_PIN     13
#define MDL_CE_START_PIN       20

#define MDL_BL_PIN   19
#define MDL_WR_PIN   18
#define MDL_CU_PIN   3
#define MDL_CUE_PIN  2
#define MDL_CLR_PIN  15


static uint8_t display_buf[4 * MDL_NUM_DISPLAYS] = {0};
static int32_t mdl_dma_chan = -1; 


void mdl2416c_init(void)
{
    gpio_init(MDL_BL_PIN);
    gpio_init(MDL_CU_PIN);
    gpio_init(MDL_CUE_PIN);
    gpio_init(MDL_CLR_PIN);

    gpio_set_dir(MDL_BL_PIN, GPIO_OUT);
    gpio_set_dir(MDL_CU_PIN, GPIO_OUT);
    gpio_set_dir(MDL_CUE_PIN, GPIO_OUT);
    gpio_set_dir(MDL_CLR_PIN, GPIO_OUT);

    gpio_put(MDL_CU_PIN, 1);
    gpio_put(MDL_CUE_PIN, 0);
    gpio_put(MDL_CLR_PIN, 1);
    gpio_put(MDL_BL_PIN, 1);

    gpio_put(MDL_CLR_PIN, 0);
    sleep_ms(500);
    gpio_put(MDL_CLR_PIN, 1);


    mdl2416c_program_init(pio0, MDL_DATA_START_PIN, MDL_ADDR_START_PIN,
                          MDL_CE_START_PIN, 4, MDL_WR_PIN);

    mdl_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(mdl_dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        mdl_dma_chan,
        &c,
        &pio0_hw->txf[MDL2416C_PIO_DISPLAY_CHAR_SM], /* Write address */
        display_buf,                                 /* Read address */
        sizeof(display_buf),
        false                                        /* Don't start yet */
    );

    // mdl2416c_print("0123456789abcdefghijklmnopqrstuvwxyz+=-/");
    mdl2416c_print("0000111122223333444455556666777788889999");
    sleep_ms(500);
    mdl2416c_print("                                        ");

}


void mdl2416c_print_buf(const uint8_t *buf, size_t buflen)
{
    dma_channel_wait_for_finish_blocking(mdl_dma_chan);

    memset(display_buf, ' ', sizeof(display_buf));
    memcpy(display_buf, buf, MIN(buflen, sizeof(display_buf)));

    for (uint16_t i = 0; i < sizeof(display_buf); i++)
    {
        uint8_t c = display_buf[i];
        
        if (c >= 'a' && c <= 'z')
        {
            display_buf[i] = c - ('a' - 'A');
        }
    }

    dma_channel_set_read_addr(mdl_dma_chan, display_buf, true);    
}


void mdl2416c_print(const uint8_t *str)
{
    mdl2416c_print_buf(str, strnlen(str, sizeof(display_buf)));
}
