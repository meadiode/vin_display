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
    0xd024, //  0: irq    wait 4          side 1     
    0xf026, //  1: set    x, 6            side 1     
    0x7061, //  2: out    null, 1         side 1     
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

// ------------- //
// drive_display //
// ------------- //

#define drive_display_wrap_target 0
#define drive_display_wrap 8

static const uint16_t drive_display_program_instructions[] = {
            //     .wrap_target
    0x6020, //  0: out    x, 32                      
    0x20c4, //  1: wait   1 irq, 4                   
    0x0041, //  2: jmp    x--, 1                     
    0x6030, //  3: out    x, 16                      
    0x6050, //  4: out    y, 16                      
    0x6010, //  5: out    pins, 16                   
    0x0046, //  6: jmp    x--, 6                     
    0x6010, //  7: out    pins, 16                   
    0x0088, //  8: jmp    y--, 8                     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program drive_display_program = {
    .instructions = drive_display_program_instructions,
    .length = 9,
    .origin = -1,
};

static inline pio_sm_config drive_display_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + drive_display_wrap_target, offset + drive_display_wrap);
    return c;
}

#include "hardware/clocks.h"
#define HDSP2000_PIO_PUSH_DATA_SM     0
#define HDSP2000_PIO_DRIVE_SM         1
#define HDSP2000_PIO_SM_MASK ((1 << HDSP2000_PIO_PUSH_DATA_SM) | \
                              (1 << HDSP2000_PIO_DRIVE_SM))
static inline void hdsp2000_program_init(PIO pio)
{
    uint8_t i;
    pio_gpio_init(pio, HDSP_CLOCK);
    pio_gpio_init(pio, HDSP_DATA_IN);
    // gpio_set_slew_rate(HDSP_CLOCK, GPIO_SLEW_RATE_SLOW);
    for (i = 0; i < 5; i++)
    {
        pio_gpio_init(pio, HDSP_COLUMN1 + i);
    }
    pio_sm_set_consecutive_pindirs(pio, HDSP2000_PIO_PUSH_DATA_SM,
                                   HDSP_CLOCK, 1, true);
    pio_sm_set_consecutive_pindirs(pio, HDSP2000_PIO_PUSH_DATA_SM,
                                   HDSP_DATA_IN, 1, true);
    pio_sm_set_consecutive_pindirs(pio, HDSP2000_PIO_DRIVE_SM,
                                   HDSP_COLUMN1, 5, true);
    pio_sm_config c;
    uint offs0 = pio_add_program(pio, &push_data_program);
    c = push_data_program_get_default_config(offs0);
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_out_pins(&c, HDSP_DATA_IN, 1);    
    sm_config_set_set_pins(&c, HDSP_DATA_IN, 1);    
    sm_config_set_sideset_pins(&c, HDSP_CLOCK);
    sm_config_set_clkdiv(&c, 20.0);
    pio_sm_init(pio, HDSP2000_PIO_PUSH_DATA_SM, offs0, &c);
    uint offs1 = pio_add_program(pio, &drive_display_program);
    c = drive_display_program_get_default_config(offs1);
    sm_config_set_out_pins(&c, HDSP_COLUMN1, NCOLUMNS);
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_clkdiv(&c, 125.0);
    pio_sm_init(pio, HDSP2000_PIO_DRIVE_SM, offs1, &c);
    pio_set_sm_mask_enabled(pio, HDSP2000_PIO_SM_MASK, true);
}

#endif

