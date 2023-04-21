
#include <string.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <pico/time.h>

#include "font5x7.h"
#include "hdsp2000.h"
#include "hdsp2000.pio.h"


#define NDISPLAYS        12
#define NCHARS           (NDISPLAYS * 4)
#define NCOLUMNS         5
#define DISPLAY_BUF_SIZE (NCOLUMNS * NCHARS)

#define HDSP_CLOCK      16
#define HDSP_DATA_IN    17
#define HDSP_COLUMN1    18
#define HDSP_COLUMN2    19
#define HDSP_COLUMN3    20
#define HDSP_COLUMN4    21
#define HDSP_COLUMN5    22

static uint8_t display_buf[DISPLAY_BUF_SIZE] = {0};
static int32_t hdsp_dma_chan = -1; 
static int32_t hdsp_dma_ctrl_chan = -1; 

static const struct {
                        const uint8_t *data;
                    } control_blocks[] = \
{
    {display_buf},
    {display_buf},
};

#define DEFAULT_TEXT "111122223333444455556666777788889999AAAABBBBCCCC"

void hdsp2000_init(void)
{
    dma_channel_config c;

    gpio_init(HDSP_CLOCK);
    gpio_init(HDSP_DATA_IN);
    gpio_init(HDSP_COLUMN1);
    gpio_init(HDSP_COLUMN2);
    gpio_init(HDSP_COLUMN3);
    gpio_init(HDSP_COLUMN4);
    gpio_init(HDSP_COLUMN5);

    pio_clear_instruction_memory(pio0);

    hdsp2000_program_init(pio0, HDSP_CLOCK, HDSP_DATA_IN, HDSP_COLUMN1);

    hdsp_dma_chan = dma_claim_unused_channel(true);
    hdsp_dma_ctrl_chan = dma_claim_unused_channel(true);

    c = dma_channel_get_default_config(hdsp_dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);
    channel_config_set_chain_to(&c, hdsp_dma_ctrl_chan);

    dma_channel_configure(
        hdsp_dma_chan,
        &c,
        &pio0_hw->txf[HDSP2000_PIO_PUSH_DATA_SM],    /* Write address */
        display_buf,                                 /* Read address */
        sizeof(display_buf),
        false                                        /* Don't start yet */
    );

    
    /* Another DMA channel that automatically triggers the first data channel 
       when it finishes transferring the buffer to the PIO program,
       in the similar manner as in pico-examples/dma/control_blocks/control_blocks.c
     */
    c = dma_channel_get_default_config(hdsp_dma_ctrl_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_ring(&c, false, 1);

    dma_channel_configure(
        hdsp_dma_ctrl_chan,
        &c,
        &dma_hw->ch[hdsp_dma_chan].al3_read_addr_trig,   /* Write address */
        &control_blocks[0],                              /* Read address */
        1,
        false                                            /* Don't start yet */
    );

    hdsp2000_print(DEFAULT_TEXT);

    dma_start_channel_mask(1u << hdsp_dma_ctrl_chan);

}


void hdsp2000_print(const uint8_t *str)
{
    uint32_t buf_idx;
    uint8_t c;
    const uint8_t *bitmap;

    memset(display_buf, 0x00, sizeof(display_buf));

    for (uint32_t i = 0; i < strnlen(str, NCHARS); i++)
    {
        buf_idx = (NCHARS - i - 1);
        c = str[i];

        if (c < ' ' || c >= 128)
        {
            c = '?';
        }
        c -= ' ';

        bitmap = font5x7 + c * NCOLUMNS;

        for (uint32_t j = 0; j < NCOLUMNS; j++)
        {
            display_buf[buf_idx + NCHARS * j] = bitmap[j]; 
        }
    }
}


void hdsp2000_init_test(void)
{
    /* bit-banging test */

    gpio_init(HDSP_CLOCK);
    gpio_init(HDSP_DATA_IN);
    gpio_init(HDSP_COLUMN1);
    gpio_init(HDSP_COLUMN2);
    gpio_init(HDSP_COLUMN3);
    gpio_init(HDSP_COLUMN4);
    gpio_init(HDSP_COLUMN5);

    gpio_set_dir(HDSP_CLOCK, true);
    gpio_set_dir(HDSP_DATA_IN, true);
    gpio_set_dir(HDSP_COLUMN1, true);
    gpio_set_dir(HDSP_COLUMN2, true);
    gpio_set_dir(HDSP_COLUMN3, true);
    gpio_set_dir(HDSP_COLUMN4, true);
    gpio_set_dir(HDSP_COLUMN5, true);

    gpio_put(HDSP_DATA_IN, 0);
    gpio_put(HDSP_COLUMN1, 0);
    gpio_put(HDSP_COLUMN2, 0);
    gpio_put(HDSP_COLUMN3, 0);
    gpio_put(HDSP_COLUMN4, 0);
    gpio_put(HDSP_COLUMN5, 0);

    // gpio_set_drive_strength(HDSP_COLUMN1, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN2, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN3, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN4, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN5, GPIO_DRIVE_STRENGTH_12MA);

    for (;;)
    {
        for (int col = 0; col < 5; col++) {

            for (int disp = 0; disp < 12; disp++) { 
                for (int digit = 0; digit < 4; digit++) { 
                    for (int dot = 6; dot >= 0; dot--) { 
                        
                        uint8_t idx = NCHARS - (disp * 4 + digit + 1);
                        uint8_t c = DEFAULT_TEXT[idx];
                        if (c < ' ')
                        {
                            c = ' ';
                        }

                        uint8_t bit = (0 != ((font5x7[(c - ' ') * 5 + col]) & (1 << dot)));

                        gpio_put(HDSP_CLOCK, 1);
                        // sleep_us(1);

                        gpio_put(HDSP_DATA_IN, bit); 
                        
                        gpio_put(HDSP_CLOCK, 0);
                        // sleep_us(1);
                    }
                }
            }
            
            gpio_put(HDSP_COLUMN1 + col, 1);
            sleep_us(2000);
            gpio_put(HDSP_COLUMN1 + col, 0);
            // sleep_us(2000);
        }
    }
}
