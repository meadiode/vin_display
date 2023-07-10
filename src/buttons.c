
#include <stdio.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>

#include "buttons.h"

#define BUTTON_KNOB_SW_PIN 24

#define BUTTONS_TASK_PRIORITY  6

#define BUTTON_UP   1
#define BUTTON_DOWN 0

#define DEBOUNCE_SAMPLES 4
#define MIN_DEBOUNCE_SAMPLES 3

static TaskHandle_t buttons_task_handle = NULL;
static MessageBufferHandle_t msg_buf = NULL;


static void buttons_callback(uint gpio, uint32_t events)
{
    BaseType_t hiprio_woken = pdFALSE;

    if (gpio == BUTTON_KNOB_SW_PIN)
    {
        vTaskNotifyGiveIndexedFromISR(buttons_task_handle, 0, &hiprio_woken);
        portYIELD_FROM_ISR(hiprio_woken);
    }
}


static void buttons_task(void *params)
{
    
    uint8_t btn_state = BUTTON_UP;

    for (;;)
    {
        if (ulTaskNotifyTakeIndexed(0, pdTRUE, portMAX_DELAY))
        {
            uint8_t cnt = 0;
            for (int i = 0; i < DEBOUNCE_SAMPLES; i++)
            {
                if (gpio_get(BUTTON_KNOB_SW_PIN))
                {
                    cnt++;
                }
                vTaskDelay(1);
            }

            if (cnt >= MIN_DEBOUNCE_SAMPLES && btn_state == BUTTON_UP)
            {
                btn_state = BUTTON_DOWN;
                xMessageBufferSend(msg_buf, &btn_state, sizeof(btn_state), 0);
            }
            else if (cnt <= (DEBOUNCE_SAMPLES - MIN_DEBOUNCE_SAMPLES) && 
                     btn_state == BUTTON_DOWN)
            {
                btn_state = BUTTON_UP;
                xMessageBufferSend(msg_buf, &btn_state, sizeof(btn_state), 0);                
            }
        }
    }

}


void buttons_init(void)
{
    xTaskCreate(buttons_task,
                "buttons",
                256,
                NULL,
                BUTTONS_TASK_PRIORITY,
                &buttons_task_handle);    

    gpio_init(BUTTON_KNOB_SW_PIN);
    gpio_set_dir(BUTTON_KNOB_SW_PIN, false);

    msg_buf = xMessageBufferCreate(sizeof(uint8_t) * 10);

    gpio_set_irq_enabled_with_callback(BUTTON_KNOB_SW_PIN,
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                       true, &buttons_callback);
}


bool buttons_get_update(uint8_t *state)
{
    return (xMessageBufferReceive(msg_buf, state, sizeof(uint8_t), 1) > 0);
}
