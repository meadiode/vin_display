
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/adc.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>

#include "fpanel.h"
#include "mjson.h"
#include "websocket.h"

#define FPANEL_TASK_PRIORITY 5
#define FPANEL_MSG_BUF_SIZE 300

static TaskHandle_t fpanel_task_handle = NULL;
static MessageBufferHandle_t msg_buf;

static void fpanel_task(void *params);

void fpanel_init(void)
{
    gpio_init(POWER_BTN_GPIO);
    gpio_init(RESET_BTN_GPIO);
    
    gpio_set_dir(POWER_BTN_GPIO, GPIO_OUT);
    gpio_set_dir(RESET_BTN_GPIO, GPIO_OUT);
    
    gpio_put(POWER_BTN_GPIO, 0);
    gpio_put(RESET_BTN_GPIO, 0);

    adc_gpio_init(POWER_LED_SENSE_GPIO);
    adc_gpio_init(HDD_LED_SENSE_GPIO);

    msg_buf = xMessageBufferCreate(FPANEL_MSG_BUF_SIZE);

    xTaskCreate(fpanel_task,
                "fpanel",
                256,
                NULL,
                FPANEL_TASK_PRIORITY,
                &fpanel_task_handle);

}


static void fpanel_task(void *params)
{
    static uint8_t buf[128];
    uint16_t pwr_ledsense = 0;
    uint16_t hdd_ledsense = 0;
    size_t data_len;

    for (;;)
    {
        data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 10);

        while (data_len > 0)
        {
            int res;

            if (mjson_get_bool(buf, data_len, "$.btn_power_press", &res))
            {
                gpio_put(POWER_BTN_GPIO, res);
            }

            if (mjson_get_bool(buf, data_len, "$.btn_reset_press", &res))
            {
                gpio_put(RESET_BTN_GPIO, res);
            }

            data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 0);
        }

        adc_select_input(POWER_LED_SENSE_ADC_INPUT);
        pwr_ledsense = adc_read();

        adc_select_input(HDD_LED_SENSE_ADC_INPUT);
        hdd_ledsense = adc_read();

        data_len = mjson_snprintf(buf, sizeof(buf),
                                 "{%Q:%B, %Q:%B}",
                                 "power_on", (pwr_ledsense >= 2000 ? 1 : 0),
                                 "hdd_on", (hdd_ledsense >= 2000 ? 1 : 0));

        websocket_send(buf, data_len, 10);
    }
}


bool fpanel_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay)
{
    return (xMessageBufferSend(msg_buf, msg, msg_len, max_delay) > 0);
}


void fpanel_power_btn(bool down)
{

}

void fpanel_reset_btn(bool down)
{

}

