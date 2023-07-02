
#include <pico.h>
#include <pico/unique_id.h>
#include <tusb.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stream_buffer.h>
#include <string.h>

#include "usb_serial.h"
#include "mdl2416c.h"
#include "klipper/serial_irq.h"

#define USB_SERIAL_TASK_PRIORITY  4

#define TXRX_BUF_SIZE 1024

uint8_t usb_serial_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static TaskHandle_t usb_serial_task_handle = NULL;
static pico_unique_board_id_t uID;
static StreamBufferHandle_t tx_buf = NULL;
static StreamBufferHandle_t rx_buf = NULL;

static void usb_serial_task(void *params);


void usb_serial_init(void)
{
    pico_get_unique_board_id(&uID);

    for (uint32_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2; i++)
    {
        /* Byte index inside the uid array */
        uint32_t bi = i / 2;
        /* Use high nibble first to keep memory order (just cosmetics) */
        uint8_t nibble = (uID.id[bi] >> 4) & 0x0F;
        uID.id[bi] <<= 4;
        /* Binary to hex digit */
        usb_serial_id[i] = nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
    }

    tusb_init();


    xTaskCreate(usb_serial_task,
                "usb_serial",
                1024 * 4,
                NULL,
                USB_SERIAL_TASK_PRIORITY,
                &usb_serial_task_handle);

}


static void dispatch_messages(void)
{
    uint8_t buf[TXRX_BUF_SIZE];
    uint8_t data_char;
    uint32_t txlen, i;

    if (tud_cdc_connected())
    {
        if (tud_cdc_available())
        {
            txlen = tud_cdc_read(buf, sizeof(buf));
            
            // if (txlen)
            // {
            //     printf("USB serial received: ");
            // }

            // for (i = 0; i < txlen; i++)
            // {
            //     printf("0x%02x ", buf[i]);
            // }

            // if (txlen)
            // {
            //     printf("\n\n");
            // }

            xStreamBufferSend(rx_buf, buf, txlen, 1);

            txlen = xStreamBufferReceive(tx_buf, buf, sizeof(buf), 1);

            // if (txlen)
            // {
            //     printf("USB serial sending: ");
            // }


            for (i = 0; i < txlen; i++)
            {
                // printf("0x%02x ", buf[i]);
                tud_cdc_write_char(buf[i]);
            }

            // if (txlen)
            // {
            //     printf("\n\n");
            // }

            tud_cdc_write_flush();
        }
    }
}


static void usb_serial_task(void *params)
{

    tx_buf = xStreamBufferCreate(TXRX_BUF_SIZE, 1);
    rx_buf = xStreamBufferCreate(TXRX_BUF_SIZE, 1);

    for (;;)
    {

        tud_task();
        dispatch_messages();

        vTaskDelay(100);
    }

}


void usb_serial_tx(const uint8_t *buf, uint32_t buflen)
{
    xStreamBufferSend(tx_buf, buf, buflen, 1);
}


uint32_t usb_serial_rx(uint8_t *buf, uint32_t buflen)
{
    return xStreamBufferReceive(rx_buf, buf, buflen, 1);
}
