
#include <pico.h>
#include <pico/unique_id.h>
#include <tusb.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>

#include "usb_serial.h"
#include "mdl2416c.h"

#define USB_SERIAL_TASK_PRIORITY  6

uint8_t usb_serial_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static TaskHandle_t usb_serial_task_handle = NULL;
static pico_unique_board_id_t uID;

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
                256,
                NULL,
                USB_SERIAL_TASK_PRIORITY,
                &usb_serial_task_handle);

}


static void dispatch_messages(void)
{
    uint8_t tx_buf[64];

    if (tud_cdc_connected())
    {
        if (tud_cdc_available())
        {
            uint tx_len = tud_cdc_read(tx_buf, sizeof(tx_buf));
            
            if (tx_len >= 3)
            {
                mdl2416c_print(tx_buf);
            }

            for (uint32_t i = 0; i < tx_len; i++)
            {
                putchar(tx_buf[i]);
                tud_cdc_write_char(tx_buf[i]);
            }
            tud_cdc_write_flush();
        }
    }
}


static void usb_serial_task(void *params)
{

    for (;;)
    {

        tud_task();
        dispatch_messages();

        vTaskDelay(10);
    }

}
