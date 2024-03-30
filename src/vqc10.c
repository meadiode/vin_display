
#include <string.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <pico/time.h>

#include "font5x7_transposed.h"

#define NDISPLAYS        10
#define NCHARS           (NDISPLAYS * 4)
#define NROWS            7
#define DISPLAY_BUF_SIZE (NROWS * NCHARS)

#define VQC_ROW_START_PIN  6
#define VQC_ROW_NUM_PINS   7
#define VQC_ROW_PINS_MASK  (((1 << VQC_ROW_NUM_PINS) - 1) << VQC_ROW_START_PIN)

#define VQC_CS_START_PIN   14
#define VQC_CS_NUM_PINS    4
#define VQC_CS_PINS_MASK   (((1 << VQC_CS_NUM_PINS) - 1) << VQC_CS_START_PIN)

#define VQC_DATA_START_PIN 19
#define VQC_DATA_NUM_PINS  5
#define VQC_DATA_PINS_MASK (((1 << VQC_DATA_NUM_PINS) - 1) << VQC_DATA_START_PIN)

#define VQC_C_E1_PIN       2    
#define VQC_C_E2_PIN       3
#define VQC_C_E3_PIN       13

#define VQC_CTRL_START_PIN 2
#define VQC_CTRL_NUM_PINS  16
#define VQC_CTRL_NC1_PIN   4
#define VQC_CTRL_NC2_PIN   5

#include "vqc10.h"
#include "vqc10.pio.h"

static struct
{
    struct
    {
        uint32_t n_chars;

        struct
        {
            uint16_t char_sel_pins;
            uint16_t char_unsel_pins;
        
        } char_ctr[NCHARS];

        uint16_t on_cycles;
        uint16_t off_cycles;
        uint16_t row_on_pins;
        uint16_t row_off_pins;

    } row_ctr[NROWS];

} disp_ctr;

static uint8_t display_buf[DISPLAY_BUF_SIZE] = {0};

static int32_t vqc_dma_data_chan = -1; 
static int32_t vqc_dma_data_ctrl_chan = -1;
static int32_t vqc_dma_dispctrl_chan = -1; 
static int32_t vqc_dma_dispctrl_ctrl_chan = -1;

static const struct 
{
    const uint8_t *data;

} dma_data_ctrl_blocks[] = \
{
    {display_buf},
    {display_buf},
};

static const struct 
{
    const uint8_t *data;

} dma_dispctrl_ctrl_blocks[] = \
{
    {(const uint8_t*)&disp_ctr},
    {(const uint8_t*)&disp_ctr},
};


static uint16_t ctrl_pins_cfg(uint8_t demux, uint8_t cs, uint8_t row)
{
    uint16_t cfg = 0;

    switch (demux)
    {
    case 1:
        cfg |= (1 << VQC_C_E2_PIN) >> VQC_CTRL_START_PIN;
        cfg |= (1 << VQC_C_E3_PIN) >> VQC_CTRL_START_PIN;        
        break;

    case 2:
        cfg |= (1 << VQC_C_E1_PIN) >> VQC_CTRL_START_PIN;
        cfg |= (1 << VQC_C_E3_PIN) >> VQC_CTRL_START_PIN;
        break;

    case 3:
        cfg |= (1 << VQC_C_E1_PIN) >> VQC_CTRL_START_PIN;
        cfg |= (1 << VQC_C_E2_PIN) >> VQC_CTRL_START_PIN;
        break;

    default:
        cfg |= (1 << VQC_C_E1_PIN) >> VQC_CTRL_START_PIN;
        cfg |= (1 << VQC_C_E2_PIN) >> VQC_CTRL_START_PIN;
        cfg |= (1 << VQC_C_E3_PIN) >> VQC_CTRL_START_PIN;
        break;
    }

    cfg |= ((cs & 0b1111) << VQC_CS_START_PIN) >> VQC_CTRL_START_PIN;

    if (row >= 7)
    {
        cfg |= (0b1111111 << VQC_ROW_START_PIN) >> VQC_CTRL_START_PIN;
    }
    else
    {
        cfg |= ((~(1 << row) & 0b1111111) << 
                            VQC_ROW_START_PIN) >> VQC_CTRL_START_PIN;
    }

    return cfg;
}

static void init_ctrl_struct(void)
{

    for (uint8_t row = 0; row < NROWS; row++)
    {
        disp_ctr.row_ctr[row].n_chars = NCHARS - 1;
        disp_ctr.row_ctr[row].on_cycles = 256;
        disp_ctr.row_ctr[row].off_cycles = 3840;
        disp_ctr.row_ctr[row].row_on_pins = ctrl_pins_cfg(0, 0, row);
        disp_ctr.row_ctr[row].row_off_pins = ctrl_pins_cfg(0, 0, 0xff);

        for (uint32_t ct = 0; ct < NCHARS; ct++)
        {
            uint8_t demux = ct / 16 + 1;
            uint8_t cs = ct % 16;

            disp_ctr.row_ctr[row].char_ctr[ct].char_sel_pins = 
                                                ctrl_pins_cfg(demux, cs, 0xff);
            disp_ctr.row_ctr[row].char_ctr[ct].char_unsel_pins = 
                                                ctrl_pins_cfg(0, 0, 0xff);
        }

    }

}


static void vqc10_init_dma(void)
{
    dma_channel_config cfg;

    vqc_dma_data_chan = dma_claim_unused_channel(true);
    vqc_dma_data_ctrl_chan = dma_claim_unused_channel(true);
    vqc_dma_dispctrl_chan = dma_claim_unused_channel(true);
    vqc_dma_dispctrl_ctrl_chan = dma_claim_unused_channel(true);


    cfg = dma_channel_get_default_config(vqc_dma_data_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_PIO0_TX0);
    channel_config_set_chain_to(&cfg, vqc_dma_data_ctrl_chan);

    dma_channel_configure(
        vqc_dma_data_chan,
        &cfg,
        &pio0_hw->txf[VQC10_PIO_PUSH_DATA_SM],       /* Write address */
        display_buf,                                 /* Read address */
        sizeof(display_buf) / 4,
        false                                        /* Don't start yet */
    );


    /* Another DMA channel that automatically triggers the first data channel 
       when it finishes transferring the buffer to the PIO program,
       in the similar manner as in pico-examples/dma/control_blocks/control_blocks.c
     */
    cfg = dma_channel_get_default_config(vqc_dma_data_ctrl_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_ring(&cfg, false, 1);

    dma_channel_configure(
        vqc_dma_data_ctrl_chan,
        &cfg,
        &dma_hw->ch[vqc_dma_data_chan].al3_read_addr_trig, /* Write address */
        &dma_data_ctrl_blocks[0],                          /* Read address */
        1,
        false                                              /* Don't start yet */
    );


    cfg = dma_channel_get_default_config(vqc_dma_dispctrl_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_PIO0_TX1);
    channel_config_set_chain_to(&cfg, vqc_dma_dispctrl_ctrl_chan);

    dma_channel_configure(
        vqc_dma_dispctrl_chan,
        &cfg,
        &pio0_hw->txf[VQC10_PIO_CTRL_SEQ_SM],        /* Write address */
        &disp_ctr,                                   /* Read address */
        sizeof(disp_ctr) / 4,
        false                                        /* Don't start yet */
    );

    cfg = dma_channel_get_default_config(vqc_dma_dispctrl_ctrl_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_ring(&cfg, false, 1);

    dma_channel_configure(
        vqc_dma_dispctrl_ctrl_chan,
        &cfg,
        &dma_hw->ch[vqc_dma_dispctrl_chan].al3_read_addr_trig, /* Write address */
        &dma_dispctrl_ctrl_blocks[0],                          /* Read address */
        1,
        false                                                  /* Don't start yet */
    );

    dma_start_channel_mask((1u << vqc_dma_data_ctrl_chan) | 
                           (1u << vqc_dma_dispctrl_ctrl_chan));
}


void vqc10_init(void)
{
    gpio_init_mask(VQC_ROW_PINS_MASK);
    gpio_set_dir_out_masked(VQC_ROW_PINS_MASK);
    gpio_put_masked(VQC_ROW_PINS_MASK, VQC_ROW_PINS_MASK);

    gpio_pull_up(VQC_ROW_START_PIN + 0);
    gpio_pull_up(VQC_ROW_START_PIN + 1);
    gpio_pull_up(VQC_ROW_START_PIN + 2);
    gpio_pull_up(VQC_ROW_START_PIN + 3);
    gpio_pull_up(VQC_ROW_START_PIN + 4);
    gpio_pull_up(VQC_ROW_START_PIN + 5);
    gpio_pull_up(VQC_ROW_START_PIN + 6);

    gpio_init_mask(VQC_CS_PINS_MASK);
    gpio_set_dir_out_masked(VQC_CS_PINS_MASK);
    gpio_put_masked(VQC_CS_PINS_MASK, 0u);

    gpio_init_mask(VQC_DATA_PINS_MASK);
    gpio_set_dir_out_masked(VQC_DATA_PINS_MASK);
    gpio_put_masked(VQC_DATA_PINS_MASK, 0u);

    gpio_init(VQC_C_E1_PIN);
    gpio_init(VQC_C_E2_PIN);
    gpio_init(VQC_C_E3_PIN);

    gpio_set_dir(VQC_C_E1_PIN, true);
    gpio_set_dir(VQC_C_E2_PIN, true);
    gpio_set_dir(VQC_C_E3_PIN, true);

    gpio_put(VQC_C_E1_PIN, true);
    gpio_put(VQC_C_E2_PIN, true);
    gpio_put(VQC_C_E3_PIN, true);

    init_ctrl_struct();

    // vqc10_bitbang_test();

    vqc10_program_init(pio0);

    vqc10_init_dma();

    // vqc10_print("1111222233334444555566667777888899990000");
    vqc10_print("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmn");

    uint32_t data_pos = 0;
    uint32_t ctr_pos = 0;
}


void vqc10_print_buf(const uint8_t *buf, size_t buflen)
{
    memset(display_buf, 0, DISPLAY_BUF_SIZE);

    for (uint8_t i = 0; i < NROWS; i++)
    {
        for (uint32_t j = 0; j < buflen; j++)
        {
            const uint8_t *ch_data = &font5x7[NROWS * buf[j]];
            display_buf[i * NCHARS + j] = ch_data[i];
        }
    }
}


void vqc10_print(const uint8_t *str)
{
    vqc10_print_buf(str, strnlen(str, NCHARS));
}


void vqc10_set_brightness(uint8_t brightness)
{
    uint16_t oncs = (4096 / 256) * brightness;
    uint16_t offcs = 4096 - oncs;

    for (uint8_t row = 0; row < NROWS; row++)
    {
        disp_ctr.row_ctr[row].on_cycles = oncs;
        disp_ctr.row_ctr[row].off_cycles = offcs;
    }
}


void vqc10_bitbang_test(void)
{
    gpio_init_mask(VQC_ROW_PINS_MASK);
    gpio_set_dir_out_masked(VQC_ROW_PINS_MASK);
    gpio_put_masked(VQC_ROW_PINS_MASK, VQC_ROW_PINS_MASK);

    gpio_pull_up(VQC_ROW_START_PIN + 0);
    gpio_pull_up(VQC_ROW_START_PIN + 1);
    gpio_pull_up(VQC_ROW_START_PIN + 2);
    gpio_pull_up(VQC_ROW_START_PIN + 3);
    gpio_pull_up(VQC_ROW_START_PIN + 4);
    gpio_pull_up(VQC_ROW_START_PIN + 5);
    gpio_pull_up(VQC_ROW_START_PIN + 6);

    gpio_init_mask(VQC_CS_PINS_MASK);
    gpio_set_dir_out_masked(VQC_CS_PINS_MASK);
    gpio_put_masked(VQC_CS_PINS_MASK, 0u);

    gpio_init_mask(VQC_DATA_PINS_MASK);
    gpio_set_dir_out_masked(VQC_DATA_PINS_MASK);
    gpio_put_masked(VQC_DATA_PINS_MASK, 0u);

    gpio_init(VQC_C_E1_PIN);
    gpio_init(VQC_C_E2_PIN);
    gpio_init(VQC_C_E3_PIN);

    gpio_set_dir(VQC_C_E1_PIN, true);
    gpio_set_dir(VQC_C_E2_PIN, true);
    gpio_set_dir(VQC_C_E3_PIN, true);

    gpio_put(VQC_C_E1_PIN, true);
    gpio_put(VQC_C_E2_PIN, true);
    gpio_put(VQC_C_E3_PIN, true);

    static uint8_t textbuf[NCHARS] = {0};

    const uint8_t text[] = "1111222233334444555566667777888899990000";
    const uint8_t tsize = sizeof(text) - 1;
    memcpy(textbuf, text, tsize);
    uint32_t tick = 0;

    for (;;)
    {
        for (uint8_t i = 0; i < VQC_ROW_NUM_PINS; i++)
        {
            for (uint8_t j = 0; j < NCHARS; j++)
            {
                uint ce_pin;
                uint8_t c_pins = j % 16;

                if (j < 16)
                {
                    ce_pin = VQC_C_E1_PIN;
                }
                else if (j < 32)
                {
                    ce_pin = VQC_C_E2_PIN;
                }
                else
                {
                    ce_pin = VQC_C_E3_PIN;
                }

                const uint8_t *ch = &font5x7[7 * textbuf[j]];

                uint8_t crow = ch[i];

                gpio_put_masked(VQC_DATA_PINS_MASK, crow << VQC_DATA_START_PIN);

                gpio_put_masked(VQC_CS_PINS_MASK, c_pins << VQC_CS_START_PIN);
                gpio_put(ce_pin, false);
                sleep_us(1);
                gpio_put_masked(VQC_CS_PINS_MASK,
                                ~(c_pins << VQC_CS_START_PIN));                
                gpio_put(ce_pin, true);
                sleep_us(1);
            }

            gpio_put_masked(VQC_ROW_PINS_MASK,
                            ~((1 << i) << VQC_ROW_START_PIN));
            sleep_us(350);
            gpio_put_masked(VQC_ROW_PINS_MASK, VQC_ROW_PINS_MASK);
            sleep_us(50);
        }
        tick++;

        if (tick > 30)
        {
            tick = 0;
            uint8_t firstc = textbuf[0];
            memmove(textbuf, textbuf + 1, NCHARS - 1);
            textbuf[NCHARS - 1] = firstc;

        }
    }

}
