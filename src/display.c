
#include <pico/stdlib.h>
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>
#include <semphr.h>

#include "display.h"
#include "command.h"

#define DISPLAY_TASK_PRIORITY  5
#define DISPLAY_MSG_BUF_SIZE   128

#define DISPLAY_DEFAULT_BRIGHTNESS 80

static TaskHandle_t display_task_handle = NULL;
static MessageBufferHandle_t msg_buf;
static SemaphoreHandle_t buffer_draw_sem = NULL;

static void display_task(void *params);

void display_init(void)
{
    msg_buf = xMessageBufferCreate(DISPLAY_MSG_BUF_SIZE);
    buffer_draw_sem = xSemaphoreCreateBinary();

    display_set_brightness(DISPLAY_DEFAULT_BRIGHTNESS);

    xTaskCreate(display_task,
                "display",
                1024,
                NULL,
                DISPLAY_TASK_PRIORITY,
                &display_task_handle);
}


static void display_task(void *params)
{    
    static uint8_t buf[DISPLAY_MSG_BUF_SIZE];
    size_t data_len;

    for (;;)
    {
        data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 10);

        if (data_len)
        {
            uint8_t cmd_id = buf[0];

            switch (cmd_id)
            {
            case COMMAND_DISPLAY_TEXT:
                {
                    uint8_t text_length = buf[1];
                    const uint8_t *text_buf = &buf[2];
                    
                    taskENTER_CRITICAL();
                    display_print_buf(text_buf, text_length);
                    // display_swap_buffer();
                    taskEXIT_CRITICAL();
                }
                break;

            case COMMAND_SET_BRIGHTNESS:
                {
                    uint8_t br_level = buf[1];

                    taskENTER_CRITICAL();
                    display_set_brightness(br_level);
                    taskEXIT_CRITICAL();
                }
                break;

            default:;
            }
        }
    }
}


bool display_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay)
{
    return (xMessageBufferSend(msg_buf, msg, msg_len, max_delay) > 0);
}


void display_swap_buffer(void)
{
    if (xSemaphoreTake(buffer_draw_sem, 1))
    {
        while (uxSemaphoreGetCount(buffer_draw_sem) == 0)
        {
            vTaskDelay(10);
        }

        // buzzer_play_sound(800.0 , 0.2); 
    }
}


void display_buffer_draw_finished_irq(void)
{
    BaseType_t hiprio_woken = pdFALSE;

    if (buffer_draw_sem)
    {
        xSemaphoreGiveFromISR(buffer_draw_sem, &hiprio_woken);
        portYIELD_FROM_ISR(hiprio_woken);
    }
}


bool display_swap_buffer_requested(void)
{
    return (buffer_draw_sem && 
            uxQueueMessagesWaitingFromISR((QueueHandle_t)buffer_draw_sem) == 0);
}