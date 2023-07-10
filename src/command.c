#include <FreeRTOS.h>
#include <task.h>
#include <stream_buffer.h>
#include <string.h>
#include <stdio.h>

#include "command.h"
#include "usb_serial.h"
#include "display.h"
#include "knob.h"
#include "buttons.h"

#define COMMAND_TASK_PRIORITY  5


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


static TaskHandle_t command_task_handle = NULL;


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


static void dispatch_incoming_data(void)
{
    static uint8_t buf[128];
    uint32_t cur_size = 0, head, size;

    size = usb_serial_rx(buf + cur_size, sizeof(buf) - cur_size);

    if (size)
    {
        cur_size += size;

        for (head = 0; head < cur_size; head++)
        {
            int16_t res = dispatch_packet(buf + head, cur_size - head);
            
            if (res == PACKET_NEED_BYTES)
            {
                if (head)
                {
                    cur_size -= head;
                    memmove(buf, buf + head, cur_size);
                    head = 0;
                }
                break;
            }
            else if (res >= MESSAGE_MIN)
            {
                head += res;
                if (head < cur_size)
                {
                    head--;
                }
                else
                {
                    cur_size = 0;
                    head = 0;
                    break;
                }
            };
        }

        if (head)
        {
            cur_size -= head;
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



static void process_outgoing_data(void)
{

    uint8_t buf[128];
    int16_t knob_pos;
    uint8_t btn_state;
    uint32_t size;

    while (knob_get_pos_update(&knob_pos))
    {
        printf("Knob position: %d\n", knob_pos);
        size = pack_knob_event(knob_pos, buf);
        usb_serial_tx(buf, size);
    }

    while (buttons_get_update(&btn_state))
    {
        printf("Button state: %u\n", btn_state);
        size = pack_button_event(btn_state, buf);
        usb_serial_tx(buf, size);
    }
}


static void command_task(void *params)
{
    for (;;)
    {
        vTaskDelay(100);

        dispatch_incoming_data();

        process_outgoing_data();
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
