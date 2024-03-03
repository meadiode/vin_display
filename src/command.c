#include <FreeRTOS.h>
#include <task.h>
#include <stream_buffer.h>
#include <string.h>
#include <stdio.h>

#include "command.h"
#include "usb_serial.h"
#include "if_uart.h"
#include "display.h"
#include "knob.h"
#include "buttons.h"
#include "buzzer.h"

#define COMMAND_TASK_PRIORITY  7

#define CMD_OK     0
#define CMD_ERROR -1

#define PACKET_NEED_BYTES         -1
#define PACKET_DROP_SYNC          -2
#define PACKET_ERROR              -2
#define PACKET_ERROR_MESSAGE_SIZE -3
#define PACKET_ERROR_NO_SYNC      -4
#define PACKET_ERROR_CRC          -5
#define PACKET_ERROR_CMD          -6

#define MESSAGE_MIN          5
#define MESSAGE_MAX          64
#define MESSAGE_TRAILER_SIZE 3
#define MESSAGE_SYNC         0x7e

#define COMM_CHANNEL_BUF_SIZE 128
#define COMM_NUM_CHANNELS    2


static TaskHandle_t command_task_handle = NULL;


typedef struct
{
    uint32_t (*rx_fun) (uint8_t*, uint32_t);
    void (*tx_fun) (const uint8_t*, uint32_t);
    uint32_t buf_cur_size;
    uint8_t buf[COMM_CHANNEL_BUF_SIZE];

} comm_channel_t; 


static comm_channel_t comm_channels[2] = 
    {
        {
            .rx_fun = usb_serial_rx,
            .tx_fun = usb_serial_tx,
            .buf_cur_size = 0,
            .buf = {0},
        },
        {
            .rx_fun = if_uart_rx,
            .tx_fun = if_uart_tx,
            .buf_cur_size = 0,
            .buf = {0},
        },
    };


uint16_t crc16_ccitt(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xffff;
    while (len--)
    {
        uint8_t data = *buf++;
        data ^= crc & 0xff;
        data ^= data << 4;
        crc = ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4)
               ^ ((uint16_t)data << 3));
    }
    return crc;
}



static int8_t dispatch_command(const uint8_t *buf, uint32_t size)
{
    uint8_t cmd_id = buf[0];

    switch (cmd_id)
    {

    case COMMAND_DISPLAY_TEXT:
        display_send(buf, size, 10);
        break;

    case COMMAND_SET_BRIGHTNESS:
        display_send(buf, size, 10);
        break;

    case COMMAND_BUZZER_PLAY:
        buzzer_send(buf, size, 10);
        break;

    case COMMAND_BUZZER_QUEUE:
        buzzer_send(buf, size, 10);
        break;

    default:
        return CMD_ERROR;
    }


    return CMD_OK;
}


static int16_t dispatch_packet(const uint8_t *buf, uint32_t size)
{
    uint8_t packet_size = buf[0];

    if (packet_size == MESSAGE_SYNC)
    {
        return PACKET_DROP_SYNC;
    }

    if (packet_size < MESSAGE_MIN || packet_size > MESSAGE_MAX)
    {
        return PACKET_ERROR_MESSAGE_SIZE;
    }

    if (packet_size > size)
    {
        return PACKET_NEED_BYTES;
    }

    if (buf[packet_size - 1] != MESSAGE_SYNC)
    {
        return PACKET_ERROR_NO_SYNC;
    }

    uint16_t crc = crc16_ccitt(buf, packet_size - MESSAGE_TRAILER_SIZE);
    uint16_t msg_crc = buf[packet_size - MESSAGE_TRAILER_SIZE] << 8;
    msg_crc |= buf[packet_size - MESSAGE_TRAILER_SIZE + 1];

    if (msg_crc != crc)
    {
        return PACKET_ERROR_CRC;
    }

    if (dispatch_command(&buf[1], size - MESSAGE_TRAILER_SIZE - 1) != CMD_OK)
    {
        return PACKET_ERROR_CMD;
    }

    return packet_size;
}


static void dispatch_incoming_data(comm_channel_t *chan)
{
    uint8_t *buf = chan->buf;
    uint32_t *cur_size = &chan->buf_cur_size;
    uint32_t head, size;

    size = chan->rx_fun(buf + *cur_size, COMM_CHANNEL_BUF_SIZE - *cur_size);

    if (size)
    {
        *cur_size += size;

        for (head = 0; head < *cur_size; head++)
        {
            int16_t res = dispatch_packet(buf + head, *cur_size - head);
            
            if (res == PACKET_NEED_BYTES)
            {
                if (head)
                {
                    *cur_size -= head;
                    memmove(buf, buf + head, *cur_size);
                    head = 0;
                }
                break;
            }
            else if (res >= MESSAGE_MIN)
            {
                head += res;
                if (head < *cur_size)
                {
                    head--;
                }
                else
                {
                    *cur_size = 0;
                    head = 0;
                    break;
                }
            }
        }

        if (head)
        {
            *cur_size -= head;
        }            
    }
}


static uint32_t pack_knob_event(int16_t knob_pos, uint8_t *buf)
{
    uint16_t crc;

    buf[0] = 7;
    buf[1] = COMMAND_KNOB_EVENT;
    buf[2] = knob_pos & 0xff;
    buf[3] = (knob_pos >> 8) & 0xff;
    crc = crc16_ccitt(buf, 4);
    buf[4] = (crc >> 8) & 0xff;
    buf[5] = crc & 0xff;
    buf[6] = MESSAGE_SYNC;

    return buf[0];
}


static uint32_t pack_button_event(uint8_t btn_state, uint8_t *buf)
{
    uint16_t crc;

    buf[0] = 6;
    buf[1] = COMMAND_BUTTON_EVENT;
    buf[2] = btn_state;
    crc = crc16_ccitt(buf, 3);
    buf[3] = (crc >> 8) & 0xff;
    buf[4] = crc & 0xff;
    buf[5] = MESSAGE_SYNC;

    return buf[0];
}


static uint32_t pack_buzzer_event(uint8_t buzzer_event, uint8_t *buf)
{
    uint16_t crc;

    buf[0] = 6;
    buf[1] = COMMAND_BUZZER_EVENT;
    buf[2] = buzzer_event;
    crc = crc16_ccitt(buf, 3);
    buf[3] = (crc >> 8) & 0xff;
    buf[4] = crc & 0xff;
    buf[5] = MESSAGE_SYNC;

    return buf[0];
}

static void tx_data(const uint8_t *buf, uint32_t buflen)
{
    for (uint8_t cn_id = 0; cn_id < COMM_NUM_CHANNELS; cn_id++)
    {
        comm_channels[cn_id].tx_fun(buf, buflen);
    }
}


static void process_outgoing_data(void)
{

    uint8_t buf[128];
    int16_t knob_pos;
    uint8_t btn_state;
    uint8_t buzzer_event;
    uint32_t size;

    while (knob_get_pos_update(&knob_pos))
    {
        printf("Knob position: %d\n", knob_pos);
        size = pack_knob_event(knob_pos, buf);
        tx_data(buf, size);
    }

    while (buttons_get_update(&btn_state))
    {
        printf("Button state: %u\n", btn_state);
        size = pack_button_event(btn_state, buf);
        tx_data(buf, size);
    }

    while (buzzer_get_update(&buzzer_event))
    {
        printf("Buzzer event: %d\n", buzzer_event);
        size = pack_buzzer_event(buzzer_event, buf);
        tx_data(buf, size);
    }
}


static void command_task(void *params)
{
    for (;;)
    {
        for (uint8_t cn_id = 0; cn_id < COMM_NUM_CHANNELS; cn_id++)
        {
            dispatch_incoming_data(&comm_channels[cn_id]);
        }
        process_outgoing_data();
        vTaskDelay(10);
    }
}


void command_init(void)
{
    xTaskCreate(command_task,
                "command",
                1024 * 4,
                NULL,
                COMMAND_TASK_PRIORITY,
                &command_task_handle);
}
