// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// --------- //
// push_data //
// --------- //

#define push_data_wrap_target 0
#define push_data_wrap 4

static const uint16_t push_data_program_instructions[] = {
            //     .wrap_target
    0xe026, //  0: set    x, 6            side 0     
    0x6001, //  1: out    pins, 1         side 0     
    0xc027, //  2: irq    wait 7          side 0     
    0x7001, //  3: out    pins, 1         side 1     
    0x0043, //  4: jmp    x--, 3          side 0     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program push_data_program = {
    .instructions = push_data_program_instructions,
    .length = 5,
    .origin = -1,
};

static inline pio_sm_config push_data_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + push_data_wrap_target, offset + push_data_wrap);
    sm_config_set_sideset(&c, 1, false, false);
    return c;
}
#endif

// ---------------- //
// count_characters //
// ---------------- //

#define count_characters_wrap_target 0
#define count_characters_wrap 8

static const uint16_t count_characters_program_instructions[] = {
            //     .wrap_target
    0xe02b, //  0: set    x, 11                      
    0xe043, //  1: set    y, 3                       
    0xe000, //  2: set    pins, 0                    
    0x20c7, //  3: wait   1 irq, 7                   
    0x0083, //  4: jmp    y--, 3                     
    0xe043, //  5: set    y, 3                       
    0x0043, //  6: jmp    x--, 3                     
    0xc025, //  7: irq    wait 5                     
    0x20c6, //  8: wait   1 irq, 6                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program count_characters_program = {
    .instructions = count_characters_program_instructions,
    .length = 9,
    .origin = -1,
};

static inline pio_sm_config count_characters_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + count_characters_wrap_target, offset + count_characters_wrap);
    return c;
}
#endif

// ----------- //
// fire_column //
// ----------- //

#define fire_column_wrap_target 0
#define fire_column_wrap 14

static const uint16_t fire_column_program_instructions[] = {
            //     .wrap_target
    0x20c5, //  0: wait   1 irq, 5                   
    0xfe01, //  1: set    pins, 1                [30]
    0xc026, //  2: irq    wait 6                     
    0x20c5, //  3: wait   1 irq, 5                   
    0xfe02, //  4: set    pins, 2                [30]
    0xc026, //  5: irq    wait 6                     
    0x20c5, //  6: wait   1 irq, 5                   
    0xfe04, //  7: set    pins, 4                [30]
    0xc026, //  8: irq    wait 6                     
    0x20c5, //  9: wait   1 irq, 5                   
    0xfe08, // 10: set    pins, 8                [30]
    0xc026, // 11: irq    wait 6                     
    0x20c5, // 12: wait   1 irq, 5                   
    0xfe10, // 13: set    pins, 16               [30]
    0xc026, // 14: irq    wait 6                     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program fire_column_program = {
    .instructions = fire_column_program_instructions,
    .length = 15,
    .origin = -1,
};

static inline pio_sm_config fire_column_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + fire_column_wrap_target, offset + fire_column_wrap);
    return c;
}

#include "hardware/clocks.h"
#define HDSP2000_PIO_PUSH_DATA_SM          0
#define HDSP2000_PIO_COUNT_CHARACTERS_SM   1
#define HDSP2000_PIO_FIRE_COLUMNS_SM       2
static inline void hdsp2000_program_init(PIO pio, uint clock_pin,
                                         uint data_pin, uint column_pins_base)
{
    uint8_t i;
    pio_gpio_init(pio, clock_pin);
    pio_gpio_init(pio, data_pin);
    gpio_set_slew_rate(clock_pin, GPIO_SLEW_RATE_SLOW);
    for (i = 0; i < 5; i++)
    {
        pio_gpio_init(pio, column_pins_base + i);
    }
    pio_sm_set_consecutive_pindirs(pio, HDSP2000_PIO_PUSH_DATA_SM,
                                   clock_pin, 1, true);
    pio_sm_set_consecutive_pindirs(pio, HDSP2000_PIO_PUSH_DATA_SM,
                                   data_pin, 1, true);
    pio_sm_set_consecutive_pindirs(pio, HDSP2000_PIO_COUNT_CHARACTERS_SM,
                                   column_pins_base, 5, true);    
    pio_sm_set_consecutive_pindirs(pio, HDSP2000_PIO_FIRE_COLUMNS_SM,
                                   column_pins_base, 5, true);
    pio_sm_config c;
    uint offs0 = pio_add_program(pio, &push_data_program);
    c = push_data_program_get_default_config(offs0);
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_out_pins(&c, data_pin, 1);    
    sm_config_set_sideset_pins(&c, clock_pin);
    sm_config_set_clkdiv(&c, 125.0 / 2.0 / 3.0);
    pio_sm_init(pio, HDSP2000_PIO_PUSH_DATA_SM, offs0, &c);
    uint offs1 = pio_add_program(pio, &count_characters_program);
    c = count_characters_program_get_default_config(offs1);
    sm_config_set_set_pins(&c, column_pins_base, 5);
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_clkdiv(&c, 1.0);
    pio_sm_init(pio, HDSP2000_PIO_COUNT_CHARACTERS_SM, offs1, &c);
    uint offs2 = pio_add_program(pio, &fire_column_program);
    c = fire_column_program_get_default_config(offs2);
    sm_config_set_set_pins(&c, column_pins_base, 5);
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_clkdiv(&c, 8000.0);
    pio_sm_init(pio, HDSP2000_PIO_FIRE_COLUMNS_SM, offs2, &c);
    pio_set_sm_mask_enabled(pio, 0x07, true);
}

#endif

