
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
#include "mdl2416c.h"

#define FPANEL_TASK_PRIORITY 5
#define FPANEL_MSG_BUF_SIZE 300

static TaskHandle_t fpanel_task_handle = NULL;
static MessageBufferHandle_t msg_buf;

static void fpanel_task(void *params);

void fpanel_init(void)
{
    // gpio_init(POWER_BTN_GPIO);
    // gpio_init(RESET_BTN_GPIO);
    
    // gpio_set_dir(POWER_BTN_GPIO, GPIO_OUT);
    // gpio_set_dir(RESET_BTN_GPIO, GPIO_OUT);
    
    // gpio_put(POWER_BTN_GPIO, 0);
    // gpio_put(RESET_BTN_GPIO, 0);

    adc_gpio_init(POWER_LED_SENSE_GPIO);
    adc_gpio_init(HDD_LED_SENSE_GPIO);
    adc_set_temp_sensor_enabled(true);

    msg_buf = xMessageBufferCreate(FPANEL_MSG_BUF_SIZE);

    xTaskCreate(fpanel_task,
                "fpanel",
                256,
                NULL,
                FPANEL_TASK_PRIORITY,
                &fpanel_task_handle);

}

static float read_onboard_temperature(void)
{
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversion_factor = 3.3f / (1 << 12);

    adc_select_input(TEMP_ADC_INPUT);

    uint16_t adc_val = adc_read();
    float adc = (float)adc_val * conversion_factor;
    float temp_c = 27.0f - (adc - 0.706f) / 0.001721f;

    return temp_c;
}


static void fpanel_task(void *params)
{
    static uint8_t buf[128];
    uint16_t pwr_ledsense = 0;
    uint16_t hdd_ledsense = 0;
    size_t data_len;
    uint16_t temp_samples = 0;
    float temp_c = 0.0;
    float temp_sum = 0.0;
    bool pc_powered = false;
    bool hdd_active = false;
    uint32_t pc_start_time = 0;
    uint32_t pc_uptime = 0;
    uint8_t dbuf[16];

    for (;;)
    {
        data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 10);

        while (data_len > 0)
        {
            int res;

            if (mjson_get_bool(buf, data_len, "$.btn_power_press", &res))
            {
                // if (res)
                // {
                //     mdl2416c_print("BPWR");
                // }
                // else
                // {
                //     mdl2416c_print("");
                // }
                // gpio_put(POWER_BTN_GPIO, res);
            }

            if (mjson_get_bool(buf, data_len, "$.btn_reset_press", &res))
            {
                // if (res)
                // {
                //     mdl2416c_print("BRST");
                // }
                // else
                // {
                //     mdl2416c_print("");
                // }
                // gpio_put(RESET_BTN_GPIO, res);
            }

            if (mjson_get_string(buf, data_len, "$.disp_str", dbuf, sizeof(dbuf)) != -1)
            {
                mdl2416c_print(dbuf);
            }


            data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 0);
        }

        adc_select_input(POWER_LED_SENSE_ADC_INPUT);
        pwr_ledsense = adc_read();
        
        if (pwr_ledsense >= 2000)
        {
            if (!pc_powered)
            {
                pc_start_time = xTaskGetTickCount();
            }
            pc_powered = true;
        }
        else
        {
            pc_powered = false;
            pc_uptime = 0;
        }

        if (pc_powered)
        {
            pc_uptime = (xTaskGetTickCount() - pc_start_time);
            pc_uptime /= configTICK_RATE_HZ;
        }

        adc_select_input(HDD_LED_SENSE_ADC_INPUT);
        hdd_ledsense = adc_read();
        hdd_active = hdd_ledsense >= 2000;

        temp_sum += read_onboard_temperature();
        temp_samples++;

        if (temp_samples >= 100)
        {
            temp_c = temp_sum / (float)temp_samples;
            temp_sum = 0.0;
            temp_samples = 0;
        }

        data_len = mjson_snprintf(buf, sizeof(buf),
                                 "{%Q:%B, %Q:%B, %Q:%.*g, %Q:%u}",
                                 "power_on", pc_powered,
                                 "hdd_on", hdd_active,
                                 "temp", 4, temp_c,
                                 "pc_uptime", pc_uptime);

        websocket_send(buf, data_len, 10);
    }
}


bool fpanel_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay)
{
    return (xMessageBufferSend(msg_buf, msg, msg_len, max_delay) > 0);
}
