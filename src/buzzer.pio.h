// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ------ //
// buzzer //
// ------ //

#define buzzer_wrap_target 0
#define buzzer_wrap 9

static const uint16_t buzzer_program_instructions[] = {
            //     .wrap_target
    0x80a0, //  0: pull   block           side 0     
    0x6020, //  1: out    x, 32           side 0     
    0x80a0, //  2: pull   block           side 0     
    0x60c0, //  3: out    isr, 32         side 0     
    0xa0e6, //  4: mov    osr, isr        side 0     
    0x6050, //  5: out    y, 16           side 0     
    0x1086, //  6: jmp    y--, 6          side 1     
    0x6050, //  7: out    y, 16           side 0     
    0x0088, //  8: jmp    y--, 8          side 0     
    0x0044, //  9: jmp    x--, 4          side 0     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program buzzer_program = {
    .instructions = buzzer_program_instructions,
    .length = 10,
    .origin = -1,
};

static inline pio_sm_config buzzer_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + buzzer_wrap_target, offset + buzzer_wrap);
    sm_config_set_sideset(&c, 1, false, false);
    return c;
}

#include "hardware/clocks.h"
#define BUZZER_PIO_SM 3
static inline void mdl2416c_program_init(PIO pio, uint buzzer_pin)
{
    pio_gpio_init(pio, buzzer_pin);
    pio_sm_set_consecutive_pindirs(pio, BUZZER_PIO_SM, buzzer_pin, 1, true);
    pio_sm_config c;
    uint offs0 = pio_add_program(pio, &buzzer_program);
    c = buzzer_program_get_default_config(offs0);
    sm_config_set_out_shift(&c, false, false, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_sideset_pins(&c, buzzer_pin);
    sm_config_set_clkdiv(&c, 125.0); /* 1 MHz */
    pio_sm_init(pio, BUZZER_PIO_SM, offs0, &c);
    pio_sm_set_enabled(pio, BUZZER_PIO_SM, true);
}

#endif

