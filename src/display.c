
#include <pico/stdlib.h>
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>

#include "display.h"
#include "command.h"

#define DISPLAY_TASK_PRIORITY  5
#define DISPLAY_MSG_BUF_SIZE   128

#define DISPLAY_DEFAULT_BRIGHTNESS 80

static TaskHandle_t display_task_handle = NULL;
static MessageBufferHandle_t msg_buf;

static void display_task(void *params);

void display_init(void)
{
    msg_buf = xMessageBufferCreate(DISPLAY_MSG_BUF_SIZE);

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
