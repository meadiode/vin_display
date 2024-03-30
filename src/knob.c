

#include <stdio.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>

#include "knob.h"
#include "knob.pio.h"


#if (VIN_DISP_BOARD == BOARD_MDL2416C_X10)

# define KNOB_ENC_A_PIN 16
# define KNOB_ENC_B_PIN 17

#elif (VIN_DISP_BOARD == BOARD_HDSP2112_X6)

# define KNOB_ENC_A_PIN 26
# define KNOB_ENC_B_PIN 27

#elif (VIN_DISP_BOARD == BOARD_VQC10_X10)

# define KNOB_ENC_A_PIN 26
# define KNOB_ENC_B_PIN 27

#endif

#define KNOB_PIO           pio1
#define KNOB_PIO_SYS_IRQ   PIO1_IRQ_0
#define KNOB_PIO_CCW_IRQ   4

#define KNOB_TASK_PRIORITY  6

static int16_t knob_pos = 0;
static MessageBufferHandle_t msg_buf;

static void knob_task(void *params);


void knob_irq0_handler(void)
{
    BaseType_t hiprio_woken = pdFALSE;

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

    xMessageBufferSendFromISR(msg_buf, &knob_pos, sizeof(knob_pos),
                              &hiprio_woken);
    portYIELD_FROM_ISR(hiprio_woken);
}


void knob_init(void)
{
    gpio_init(KNOB_ENC_A_PIN);
    gpio_init(KNOB_ENC_B_PIN);

    gpio_pull_up(KNOB_ENC_A_PIN);
    gpio_pull_up(KNOB_ENC_B_PIN);

    pio_set_irq0_source_enabled(KNOB_PIO, pis_interrupt0, true);
    irq_set_exclusive_handler(KNOB_PIO_SYS_IRQ, knob_irq0_handler);
    irq_set_enabled(KNOB_PIO_SYS_IRQ, true);

    knob_program_init(KNOB_PIO, KNOB_ENC_A_PIN);

    msg_buf = xMessageBufferCreate(sizeof(knob_pos) * 10);
}


bool knob_get_pos_update(int16_t *pos)
{
    return (xMessageBufferReceive(msg_buf, pos, sizeof(knob_pos), 1) > 0);
}
