

#include <stdio.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include "knob.h"
#include "knob.pio.h"
#include "buzzer.h"

#define KNOB_ENC_A_PIN 16
#define KNOB_ENC_B_PIN 17
#define KNOB_SW_PIN    24

#define KNOB_PIO           pio1
#define KNOB_PIO_SYS_IRQ   PIO1_IRQ_0
#define KNOB_PIO_CCW_IRQ   4

static int16_t knob_pos = 0;

void knob_irq0_handler(void)
{
    if (pio_interrupt_get(KNOB_PIO, KNOB_PIO_CCW_IRQ))
    {
        knob_pos--;
        pio_interrupt_clear(KNOB_PIO, KNOB_PIO_CCW_IRQ);
    }
    else
    {
        knob_pos++;
    }

    pio_interrupt_clear(KNOB_PIO, 0);

    printf("position: %d\n", knob_pos);
}

void knob_init(void)
{
    gpio_init(KNOB_ENC_A_PIN);
    gpio_init(KNOB_ENC_B_PIN);
    gpio_init(KNOB_SW_PIN);


    pio_set_irq0_source_enabled(KNOB_PIO, pis_interrupt0, true);
    irq_set_exclusive_handler(KNOB_PIO_SYS_IRQ, knob_irq0_handler);
    irq_set_enabled(KNOB_PIO_SYS_IRQ, true);

    knob_program_init(KNOB_PIO, KNOB_ENC_A_PIN);

    // for (;;)
    // {
    //     sleep_ms(1000);
    // }

}
