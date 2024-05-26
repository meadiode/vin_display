
#include <string.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <pico/time.h>
#include <FreeRTOS.h>
#include <task.h>

#include "font5x7.h"

#define NDISPLAYS        20
#define NCHARS           (NDISPLAYS * 4)
#define NCOLUMNS         5
#define NROWS            7
#define NCHARS_IN_ROW    (5 * 4)
#define DISPLAY_BUF_SIZE (NCOLUMNS * NCHARS)
#define CHAR_HIDDEN_ROWS 8
#define CHAR_HIDDEN_COLS 2

#define SCREEN_W         (NCOLUMNS + CHAR_HIDDEN_COLS) * NCHARS_IN_ROW - CHAR_HIDDEN_COLS
#define SCREEN_H         (NROWS + CHAR_HIDDEN_ROWS) * (NCHARS / NCHARS_IN_ROW) - CHAR_HIDDEN_ROWS

#define HDSP_CLOCK      6
#define HDSP_DATA_IN    2
#define HDSP_DATA_IN2   3
#define HDSP_COLUMN1    7
#define HDSP_COLUMN2    8
#define HDSP_COLUMN3    9
#define HDSP_COLUMN4    10
#define HDSP_COLUMN5    11
#define HDSP_BR         12

#include "hdsp2000.h"
#include "hdsp2000.pio.h"

static uint8_t display_buf0[DISPLAY_BUF_SIZE] = {0};
static uint8_t display_buf1[DISPLAY_BUF_SIZE] = {0};
static uint8_t *back_buf = display_buf1;

static struct
{
    uint32_t n_chars;
    uint16_t on_cycles;
    uint16_t off_cycles;
    uint16_t col_on_pins;
    uint16_t col_off_pins;

} disp_ctr[NCOLUMNS] = {0};


static int32_t hdsp_dma_chan = -1; 
static int32_t hdsp_dma_ctrl_chan = -1;
static int32_t hdsp_dma_dispctrl_chan = -1; 
static int32_t hdsp_dma_dispctrl_ctrl_chan = -1;

static struct
{
    const uint8_t *data;

} control_blocks[] = \
{
    {display_buf0},
    {display_buf0},
};

static const struct 
{
    const uint8_t *data;

} dma_dispctrl_ctrl_blocks[] = \
{
    {(const uint8_t*)disp_ctr},
    {(const uint8_t*)disp_ctr},
};

// #define DEFAULT_TEXT "111122223333444455556666777788889999AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKK"
// #define DEFAULT_TEXT "111122223333444455556666777788889999AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNNOOOOPPPP"
#define DEFAULT_TEXT "                                            DUCK                                                    "


extern void display_buffer_draw_finished_irq(void);
extern bool display_swap_buffer_requested(void);

static void init_ctrl_struct(void)
{

    for (uint8_t col = 0; col < NCOLUMNS; col++)
    {
        disp_ctr[col].n_chars = NCHARS - 1;
        disp_ctr[col].on_cycles = 254;
        disp_ctr[col].off_cycles = 255 - 254;
        disp_ctr[col].col_on_pins =  (~(1 << col)) & 0xffff;

        disp_ctr[col].col_off_pins = 0xffff;
    }

}


static void hdsp_dma_handler(void)
{
    if (display_swap_buffer_requested())
    {
        if (control_blocks[0].data == display_buf0)
        {
            control_blocks[0].data = display_buf1;
            control_blocks[1].data = display_buf1;
            back_buf = display_buf0;
        }
        else
        {
            control_blocks[0].data = display_buf0;
            control_blocks[1].data = display_buf0;
            back_buf = display_buf1;
        }
    }

    /* Clear the interrupt request. */
    dma_hw->ints1 = 1u << hdsp_dma_ctrl_chan;

    display_buffer_draw_finished_irq();
}


static void hdsp_init_dma(void)
{
    dma_channel_config cfg;

    hdsp_dma_chan = dma_claim_unused_channel(true);
    hdsp_dma_ctrl_chan = dma_claim_unused_channel(true);
    hdsp_dma_dispctrl_chan = dma_claim_unused_channel(true);
    hdsp_dma_dispctrl_ctrl_chan = dma_claim_unused_channel(true);

    cfg = dma_channel_get_default_config(hdsp_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_PIO0_TX0);
    channel_config_set_chain_to(&cfg, hdsp_dma_ctrl_chan);

    dma_channel_configure(
        hdsp_dma_chan,
        &cfg,
        &pio0_hw->txf[HDSP2000_PIO_PUSH_DATA_SM],    /* Write address */
        display_buf0,                                /* Read address */
        sizeof(display_buf0),
        false                                        /* Don't start yet */
    );
    
    /* Another DMA channel that automatically triggers the first data channel 
       when it finishes transferring the buffer to the PIO program,
       in the similar manner as in pico-examples/dma/control_blocks/control_blocks.c
     */
    cfg = dma_channel_get_default_config(hdsp_dma_ctrl_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_ring(&cfg, false, 1);

    dma_channel_configure(
        hdsp_dma_ctrl_chan,
        &cfg,
        &dma_hw->ch[hdsp_dma_chan].al3_read_addr_trig,   /* Write address */
        &control_blocks[0],                              /* Read address */
        1,
        false                                            /* Don't start yet */
    );


    cfg = dma_channel_get_default_config(hdsp_dma_dispctrl_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_PIO0_TX1);
    channel_config_set_chain_to(&cfg, hdsp_dma_dispctrl_ctrl_chan);

    dma_channel_configure(
        hdsp_dma_dispctrl_chan,
        &cfg,
        &pio0_hw->txf[HDSP2000_PIO_DRIVE_SM],        /* Write address */
        disp_ctr,                                    /* Read address */
        sizeof(disp_ctr) / 4,
        false                                        /* Don't start yet */
    );

    cfg = dma_channel_get_default_config(hdsp_dma_dispctrl_ctrl_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_ring(&cfg, false, 1);

    dma_channel_configure(
        hdsp_dma_dispctrl_ctrl_chan,
        &cfg,
        &dma_hw->ch[hdsp_dma_dispctrl_chan].al3_read_addr_trig, /* Write address */
        &dma_dispctrl_ctrl_blocks[0],                          /* Read address */
        1,
        false                                                  /* Don't start yet */
    );


    dma_channel_set_irq1_enabled(hdsp_dma_ctrl_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_1, hdsp_dma_handler);
    irq_set_enabled(DMA_IRQ_1, true);

    dma_start_channel_mask((1u << hdsp_dma_ctrl_chan) | 
                           (1u << hdsp_dma_dispctrl_ctrl_chan));    

    // dma_start_channel_mask((1u << hdsp_dma_chan) | 
    //                        (1u << hdsp_dma_dispctrl_chan));
}


void hdsp2000_init(void)
{
    // hdsp2000_init_test();

    gpio_init(HDSP_CLOCK);
    gpio_init(HDSP_DATA_IN);
    gpio_init(HDSP_COLUMN1);
    gpio_init(HDSP_COLUMN2);
    gpio_init(HDSP_COLUMN3);
    gpio_init(HDSP_COLUMN4);
    gpio_init(HDSP_COLUMN5);
    gpio_init(HDSP_BR);

    gpio_put(HDSP_COLUMN1, 1);
    gpio_put(HDSP_COLUMN2, 1);
    gpio_put(HDSP_COLUMN3, 1);
    gpio_put(HDSP_COLUMN4, 1);
    gpio_put(HDSP_COLUMN5, 1);

    gpio_set_dir(HDSP_BR, true);
    gpio_put(HDSP_BR, 0);

    pio_clear_instruction_memory(pio0);
    hdsp2000_program_init(pio0);

    init_ctrl_struct();
    hdsp_init_dma();

    hdsp2000_print("HI THERE");
}


void hdsp2000_print(const uint8_t *str)
{
    hdsp2000_print_buf(str, strnlen(str, NCHARS));
}


void hdsp2000_set_brightness(uint8_t brightness)
{

}


void hdsp2000_print_buf(const uint8_t *buf, size_t buf_len)
{
    uint32_t disp_buf_idx;
    uint8_t c;
    const uint8_t *bitmap;

    for (uint32_t i = 0; i < MIN(NCHARS, buf_len); i++)
    {
        disp_buf_idx = (NCHARS - i - 1);
        c = buf[i];
        bitmap = font5x7 + c * NCOLUMNS;

        for (uint32_t j = 0; j < NCOLUMNS; j++)
        {
            // back_buf[disp_buf_idx + NCHARS * j] = bitmap[j]; 
            display_buf0[disp_buf_idx + NCHARS * j] = bitmap[j]; 
            display_buf1[disp_buf_idx + NCHARS * j] = bitmap[j]; 
        }
    }
}


void hdsp2000_put_pixel(int16_t x, int16_t y, uint8_t val)
{
    if ((x < 0 || x >= SCREEN_W) || (y < 0 || y >= SCREEN_H))
    {
        return;
    }

    uint16_t ccol = x % (NCOLUMNS + CHAR_HIDDEN_COLS);
    uint16_t crow = y % (NROWS + CHAR_HIDDEN_ROWS);

    if (ccol >= NCOLUMNS)
    {
        return;
    }

    if (crow >= NROWS)
    {
        return;
    }

    uint16_t cx = x / (NCOLUMNS + CHAR_HIDDEN_COLS);
    uint16_t cy = y / (NROWS + CHAR_HIDDEN_ROWS);
    uint32_t idx = NCHARS - (cy * NCHARS_IN_ROW + cx) - 1 + NCHARS * ccol;

    if (val)
    {
        back_buf[idx] |= 1 << crow;
    }
    else
    {
        back_buf[idx] &= ~(1 << crow);
    }
}


void hdsp2000_clear(void)
{
    memset(back_buf, 0x00, sizeof(display_buf0));
}


uint16_t hdsp2000_width(void)
{
    return SCREEN_W;
}


uint16_t hdsp2000_height(void)
{
    return SCREEN_H;
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

    gpio_set_slew_rate(HDSP_CLOCK, GPIO_SLEW_RATE_FAST);

    gpio_set_pulls(HDSP_COLUMN1, true, false);
    gpio_set_pulls(HDSP_COLUMN2, true, false);
    gpio_set_pulls(HDSP_COLUMN3, true, false);
    gpio_set_pulls(HDSP_COLUMN4, true, false);
    gpio_set_pulls(HDSP_COLUMN5, true, false);

    gpio_put(HDSP_DATA_IN, 1);
    gpio_put(HDSP_COLUMN1, 1);
    gpio_put(HDSP_COLUMN2, 1);
    gpio_put(HDSP_COLUMN3, 1);
    gpio_put(HDSP_COLUMN4, 1);
    gpio_put(HDSP_COLUMN5, 1);

    // gpio_set_drive_strength(HDSP_COLUMN1, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN2, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN3, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN4, GPIO_DRIVE_STRENGTH_12MA);
    // gpio_set_drive_strength(HDSP_COLUMN5, GPIO_DRIVE_STRENGTH_12MA);

    for (;;)
    {
        for (int col = 0; col < 5; col++) {

            for (int disp = 0; disp < NDISPLAYS; disp++) { 
                for (int digit = 0; digit < 4; digit++) { 
                    for (int dot = 6; dot >= 0; dot--) { 
                        
                        uint8_t idx = NCHARS - (disp * 4 + digit + 1);
                        uint8_t c = DEFAULT_TEXT[idx];
                        uint8_t bit =((font5x7[c * 5 + col]) & (1 << dot));

                        // if (col == 0 && digit == 3 && dot == 3)
                        // {
                        //     bit = 1;
                        // }
                        // else
                        // {
                        //     bit = 0;
                        // }

                        gpio_put(HDSP_CLOCK, 0);
                        // gpio_put(HDSP_DATA_IN, idx == 0 ? true : false); 
                        gpio_put(HDSP_DATA_IN, bit ? 1 : 0); 
                        gpio_put(HDSP_CLOCK, 1);
                        sleep_us(1);
                        
                        gpio_put(HDSP_CLOCK, 0);
                        // sleep_us(1);
                        gpio_put(HDSP_DATA_IN, 0); 

                    }
                }
            }
            
            gpio_put(HDSP_COLUMN1 + col, 0);
            sleep_us(20);
            gpio_put(HDSP_COLUMN1 + col, 1);
            sleep_us(80);
        }
    }
}
