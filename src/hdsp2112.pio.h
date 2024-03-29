// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ------------ //
// display_char //
// ------------ //

#define display_char_wrap_target 0
#define display_char_wrap 2

static const uint16_t display_char_program_instructions[] = {
            //     .wrap_target
    0x6008, //  0: out    pins, 8                    
    0xc024, //  1: irq    wait 4                     
    0xc024, //  2: irq    wait 4                     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program display_char_program = {
    .instructions = display_char_program_instructions,
    .length = 3,
    .origin = -1,
};

static inline pio_sm_config display_char_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + display_char_wrap_target, offset + display_char_wrap);
    return c;
}
#endif

// ----------- //
// select_char //
// ----------- //

#define select_char_wrap_target 0
#define select_char_wrap 8

static const uint16_t select_char_program_instructions[] = {
            //     .wrap_target
    0xe020, //  0: set    x, 0                       
    0xa001, //  1: mov    pins, x                    
    0xc025, //  2: irq    wait 5                     
    0xc025, //  3: irq    wait 5                     
    0xa049, //  4: mov    y, !x                      
    0x0086, //  5: jmp    y--, 6                     
    0xa02a, //  6: mov    x, !y                      
    0xe048, //  7: set    y, 8                       
    0x00a1, //  8: jmp    x != y, 1                  
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program select_char_program = {
    .instructions = select_char_program_instructions,
    .length = 9,
    .origin = -1,
};

static inline pio_sm_config select_char_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + select_char_wrap_target, offset + select_char_wrap);
    return c;
}
#endif

// ------------------------------ //
// select_display_and_write_chars //
// ------------------------------ //

#define select_display_and_write_chars_wrap_target 0
#define select_display_and_write_chars_wrap 14

static const uint16_t select_display_and_write_chars_program_instructions[] = {
            //     .wrap_target
    0xf020, //  0: set    x, 0            side 1     
    0xf047, //  1: set    y, 7            side 1     
    0x30c4, //  2: wait   1 irq, 4        side 1     
    0x30c5, //  3: wait   1 irq, 5        side 1     
    0xba01, //  4: mov    pins, x         side 1 [10]
    0xaa42, //  5: nop                    side 0 [10]
    0xfa07, //  6: set    pins, 7         side 1 [10]
    0x30c4, //  7: wait   1 irq, 4        side 1     
    0x30c5, //  8: wait   1 irq, 5        side 1     
    0x1082, //  9: jmp    y--, 2          side 1     
    0xb049, // 10: mov    y, !x           side 1     
    0x108c, // 11: jmp    y--, 12         side 1     
    0xb02a, // 12: mov    x, !y           side 1     
    0xf046, // 13: set    y, 6            side 1     
    0x10a1, // 14: jmp    x != y, 1       side 1     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program select_display_and_write_chars_program = {
    .instructions = select_display_and_write_chars_program_instructions,
    .length = 15,
    .origin = -1,
};

static inline pio_sm_config select_display_and_write_chars_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + select_display_and_write_chars_wrap_target, offset + select_display_and_write_chars_wrap);
    sm_config_set_sideset(&c, 1, false, false);
    return c;
}

#include "hardware/clocks.h"
#define HDSP2112_PIO_DISPLAY_CHAR_SM       0
#define HDSP2112_PIO_SELECT_CHAR_SM        1
#define HDSP2112_PIO_SELECT_DISPLAY_SM     2
#define HDSP2112_PIO_DISP_SM_MASK ((1 << HDSP2112_PIO_DISPLAY_CHAR_SM) | \
                                   (1 << HDSP2112_PIO_SELECT_CHAR_SM) | \
                                   (1 << HDSP2112_PIO_SELECT_DISPLAY_SM))
static inline void hdsp2112_program_init(PIO pio)
{
    uint8_t i;
    for (i = 0; i < HDSP_DATA_PINS_NUM; i++)
    {
        pio_gpio_init(pio, HDSP_DATA_START_PIN + i);
    }
    for (i = 0; i < HDSP_ADDR_PINS_NUM; i++)
    {
        pio_gpio_init(pio, HDSP_ADDR_START_PIN + i);
    }
    for (i = 0; i < HDSP_CE_PINS_NUM; i++)
    {
        pio_gpio_init(pio, HDSP_CE_START_PIN + i);
    }
    pio_gpio_init(pio, HDSP_WR_PIN);
    pio_sm_set_consecutive_pindirs(pio, HDSP2112_PIO_DISPLAY_CHAR_SM,
                                   HDSP_DATA_START_PIN, HDSP_DATA_PINS_NUM,
                                   true);
    pio_sm_set_consecutive_pindirs(pio, HDSP2112_PIO_SELECT_CHAR_SM,
                                   HDSP_ADDR_START_PIN, HDSP_ADDR_PINS_NUM,
                                   true);
    pio_sm_set_consecutive_pindirs(pio, HDSP2112_PIO_SELECT_DISPLAY_SM,
                                   HDSP_CE_START_PIN, HDSP_CE_PINS_NUM,
                                   true);
    pio_sm_set_consecutive_pindirs(pio, HDSP2112_PIO_SELECT_DISPLAY_SM,
                                   HDSP_WR_PIN, 1, true);
    pio_sm_config c;
    uint offs0 = pio_add_program(pio, &display_char_program);
    c = display_char_program_get_default_config(offs0);
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_out_pins(&c, HDSP_DATA_START_PIN, HDSP_DATA_PINS_NUM);
    sm_config_set_clkdiv(&c, 4.0);
    pio_sm_init(pio, HDSP2112_PIO_DISPLAY_CHAR_SM, offs0, &c);
    uint offs1 = pio_add_program(pio, &select_char_program);
    c = select_char_program_get_default_config(offs1);
    sm_config_set_out_shift(&c, false, false, 32);
    sm_config_set_out_pins(&c, HDSP_ADDR_START_PIN, HDSP_ADDR_PINS_NUM);
    sm_config_set_clkdiv(&c, 4.0);
    pio_sm_init(pio, HDSP2112_PIO_SELECT_CHAR_SM, offs1, &c);
    uint offs2 = pio_add_program(pio, &select_display_and_write_chars_program);
    c = select_display_and_write_chars_program_get_default_config(offs2);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_out_pins(&c, HDSP_CE_START_PIN, HDSP_CE_PINS_NUM);
    sm_config_set_set_pins(&c, HDSP_CE_START_PIN, HDSP_CE_PINS_NUM);
    sm_config_set_sideset_pins(&c, HDSP_WR_PIN);
    sm_config_set_clkdiv(&c, 4.0);
    pio_sm_init(pio, HDSP2112_PIO_SELECT_DISPLAY_SM, offs2, &c);
    pio_set_sm_mask_enabled(pio, HDSP2112_PIO_DISP_SM_MASK, true);
}

#endif

