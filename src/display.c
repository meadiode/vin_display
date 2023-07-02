
#include <pico/stdlib.h>
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>

#include "display.h"
#include "mjson.h"
#include "mdl2416c.h"

#define DISPLAY_TASK_PRIORITY  5
#define DISPLAY_MSG_BUF_SIZE   300

static const uint8_t *MONTHS[12] = {"Jan", "Feb", "Mar", "Apr",
                                    "May", "Jun", "Jul", "Aug",
                                    "Sep", "Oct", "Nov", "Dec"};

static const uint8_t *DAYS[7] = {"Sun", "Mon", "Tue", "Wed",
                                 "Thu", "Fri", "Sat"};

static TaskHandle_t display_task_handle = NULL;
static MessageBufferHandle_t msg_buf;

static void display_task(void *params);

void display_init(void)
{
    adc_set_temp_sensor_enabled(true);

    msg_buf = xMessageBufferCreate(DISPLAY_MSG_BUF_SIZE);

    xTaskCreate(display_task,
                "display",
                256,
                NULL,
                DISPLAY_TASK_PRIORITY,
                &display_task_handle);

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


static void display_task(void *params)
{    
    static uint8_t buf[128];
    size_t data_len;
    uint16_t temp_samples = 0;
    float temp_c = 0.0;
    float temp_sum = 0.0;
    uint8_t dbuf[40] = {0};
    uint32_t user_msg_tmr = 0;
    datetime_t dt;

    for (;;)
    {
        data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 10);

        while (data_len > 0)
        {
            int res;

            if (mjson_get_string(buf, data_len, "$.disp_str", dbuf, sizeof(dbuf)) != -1)
            {
                user_msg_tmr = strnlen(dbuf, sizeof(dbuf)) ? 5000 : 0;
            }

            data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 0);
        }

        temp_sum += read_onboard_temperature();
        temp_samples++;

        if (temp_samples >= 100)
        {
            temp_c = temp_sum / (float)temp_samples;
            temp_sum = 0.0;
            temp_samples = 0;
        }

        data_len = mjson_snprintf(buf, sizeof(buf),
                                  "{%Q:%.*g}", "temp", 4, temp_c);

        // websocket_send(buf, data_len, 10);

        if (!user_msg_tmr)
        {
            
            rtc_get_datetime(&dt);

            // snprintf(dbuf, sizeof(dbuf), "t:%0.2fC BPWR:%u BRST:%u",
            //          temp_c, bpwr, brst);
            snprintf(dbuf, 32, "%02u:%02u:%02u t:%2.1fC %s %u %s",
                     dt.hour, dt.min, dt.sec, temp_c,
                     MONTHS[dt.month - 1], dt.day, DAYS[dt.dotw]);
        }
        else
        {
            user_msg_tmr--;
        }

        mdl2416c_print(dbuf);
    }
}


bool display_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay)
{
    return (xMessageBufferSend(msg_buf, msg, msg_len, max_delay) > 0);
}
