
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <string.h>
#include <stdio.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>

#include "buzzer.h"
#include "buzzer.pio.h"
#include "command.h"

#define BUZZER_TASK_PRIORITY        4
#define BUZZER_TASK_NOTIFY_DMA_DONE 0


#define BUZZER_PIN 25

static int32_t buzzer_dma_chan = -1;
static TaskHandle_t buzzer_task_handle = NULL;
static MessageBufferHandle_t msg_buf = NULL;
static MessageBufferHandle_t msg_out_buf = NULL;

static bool dma_active = false;

static uint8_t buffer[1024 * 8];
static uint32_t buffer_pos = 0;

extern const uint8_t no_data[];


static void buzzer_dma_handler(void)
{
    BaseType_t hiprio_woken = pdFALSE;

    /* Clear the interrupt request. */
    dma_hw->ints0 = 1u << buzzer_dma_chan;

    dma_active = false;
    vTaskNotifyGiveIndexedFromISR(buzzer_task_handle,
                                  BUZZER_TASK_NOTIFY_DMA_DONE,
                                  &hiprio_woken);
    portYIELD_FROM_ISR(hiprio_woken);
}


static void queue_sound(float freq, float duration)
{
    uint32_t ncycles = (uint32_t)(freq * duration);
    uint16_t hperiod = (uint16_t)(1000000.0 / freq) / 2;
    uint32_t cycle = (hperiod << 16) | hperiod;

    memcpy(buffer + buffer_pos, &ncycles, sizeof(ncycles));
    buffer_pos += sizeof(ncycles);
    memcpy(buffer + buffer_pos, &cycle, sizeof(cycle));
    buffer_pos += sizeof(cycle);
}


static void start_dma(void)
{
    dma_channel_set_trans_count(buzzer_dma_chan,
                                buffer_pos / sizeof(uint32_t), false);
    if (buffer_pos)
    {
        dma_active = true;
        dma_channel_set_read_addr(buzzer_dma_chan, buffer, true);
        buffer_pos = 0;
    }
}


static void buzzer_task(void *params)
{
    static uint8_t buf[128];
    uint8_t buf_length;
    size_t data_len;
    uint8_t event = BUZZER_EVENT_BUFFER_NONE;

    queue_sound(1000.0, 0.25);
    queue_sound(1200.0, 0.25);
    queue_sound(1400.0, 0.25);
    start_dma();

    for (;;)
    {
        data_len = xMessageBufferReceive(msg_buf, buf, sizeof(buf), 10);

        if (data_len)
        {
            uint8_t cmd_id = buf[0];

            switch (cmd_id)
            {
            case COMMAND_BUZZER_PLAY:
                {                                        
                    if (dma_active)
                    {
                        taskENTER_CRITICAL();
                        dma_channel_abort(buzzer_dma_chan);
                        taskEXIT_CRITICAL();

                        /* Wait till the interrupt finishes */
                        ulTaskNotifyTakeIndexed(BUZZER_TASK_NOTIFY_DMA_DONE,
                                                pdTRUE, portMAX_DELAY);
                    }

                    /* Check the cancel flag, if set then restart the SM */
                    if (buf[1])
                    {
                        pio_sm_restart(pio1, BUZZER_PIO_SM);
                    }

                    dma_channel_set_trans_count(buzzer_dma_chan,
                                                buffer_pos / sizeof(uint32_t),
                                                false);
                    if (buffer_pos)
                    {
                        taskENTER_CRITICAL();
                        dma_active = true;
                        dma_channel_set_read_addr(buzzer_dma_chan,
                                                  buffer, true);
                        taskEXIT_CRITICAL();
                        buffer_pos = 0;
                    }
                }
                break;

            case COMMAND_BUZZER_QUEUE:
                {
                    buf_length = buf[1];
                    if (buf_length < sizeof(uint32_t) * 2)
                    {
                        break;
                    }
                    buf_length = MIN(sizeof(buffer) - buffer_pos, buf_length);

                    if (buf_length == 0)
                    {
                        event = BUZZER_EVENT_BUFFER_FULL;
                        xMessageBufferSend(msg_out_buf, &event,
                                           sizeof(uint8_t), 10);
                        break;
                    }

                    memcpy(buffer + buffer_pos, buf + 2, buf_length);
                    buffer_pos += buf_length;
                }
                break;

            default:;
            }
        }

        if (ulTaskNotifyTakeIndexed(BUZZER_TASK_NOTIFY_DMA_DONE, pdTRUE, 0))
        {
            event = BUZZER_EVENT_BUFFER_DONE;
            xMessageBufferSend(msg_out_buf, &event, sizeof(uint8_t), 10);
        }

    }
}


void buzzer_init(void)
{
    dma_channel_config cfg;

    
    xTaskCreate(buzzer_task,
                "buzzer",
                256,
                NULL,
                BUZZER_TASK_PRIORITY,
                &buzzer_task_handle);        

    msg_buf = xMessageBufferCreate(sizeof(uint32_t) * 32);
    msg_out_buf = xMessageBufferCreate(sizeof(uint8_t) * 16);

    gpio_init(BUZZER_PIN);

    buzzer_program_init(pio1, BUZZER_PIN);

    buzzer_dma_chan = dma_claim_unused_channel(true);
    cfg = dma_channel_get_default_config(buzzer_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_PIO1_TX3);

    dma_channel_configure(
        buzzer_dma_chan,
        &cfg,
        &pio1_hw->txf[BUZZER_PIO_SM],                /* Write address */
        buffer,                                      /* Read address */
        0,
        false                                        /* Don't start yet */
    );

    dma_channel_set_irq0_enabled(buzzer_dma_chan, true);

    irq_set_exclusive_handler(DMA_IRQ_0, buzzer_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}


bool buzzer_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay)
{
    return (xMessageBufferSend(msg_buf, msg, msg_len, max_delay) > 0);
}


bool buzzer_get_update(uint8_t *event)
{
    return (xMessageBufferReceive(msg_out_buf, event, sizeof(uint8_t), 10) > 0);
}


void buzzer_queue_sound(float freq, float duration)
{
    static struct __attribute__ ((packed))
    {
        uint8_t  cmd;
        uint8_t  len;
        uint32_t ncycles;
        uint16_t on_period;
        uint16_t off_period;
    
    } quecmd = 
    {
        .cmd = COMMAND_BUZZER_QUEUE,
        .len = 8,
    };


    quecmd.ncycles = (uint32_t)(freq * duration);
    quecmd.on_period = (uint16_t)(1000000.0 / freq) / 2;
    quecmd.off_period = quecmd.on_period;

    buzzer_send((const uint8_t*)&quecmd, sizeof(quecmd), 10);
}


void buzzer_play_queue(bool cancel_current_sound)
{
    static struct __attribute__ ((packed))
    {
        uint8_t cmd;
        uint8_t cancel;
    } playcmd =
    {
        .cmd = COMMAND_BUZZER_PLAY,
        .cancel = 0,
    };

    playcmd.cancel = cancel_current_sound;

    buzzer_send((const uint8_t*)&playcmd, sizeof(playcmd), 10);
}


void buzzer_play_sound(float freq, float duration)
{
    buzzer_queue_sound(freq, duration);
    buzzer_play_queue(true);
}