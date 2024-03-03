
#include <pico.h>
#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stream_buffer.h>
#include <string.h>

#include "if_uart.h"

#define IF_UART_TASK_PRIORITY  4

#define IF_UART_ID     uart1
#define IF_UART_IRQ    UART1_IRQ
#define IF_UART_TX_PIN 4
#define IF_UART_RX_PIN 5

#define BAUD_RATE      115200
#define DATA_BITS      8
#define STOP_BITS      1
#define PARITY         UART_PARITY_NONE
#define TXRX_BUF_SIZE  1024

static TaskHandle_t if_uart_task_handle = NULL;
static StreamBufferHandle_t tx_buf = NULL;
static StreamBufferHandle_t rx_buf = NULL;

static void if_uart_task(void *params);


static void if_uart_irq_handler(void)
{
    uint8_t c;
    BaseType_t hiprio_woken = pdFALSE;

    while (uart_is_readable(IF_UART_ID))
    {
        c = uart_getc(IF_UART_ID);
        xStreamBufferSendFromISR(rx_buf, &c, 1, &hiprio_woken);
    }

    portYIELD_FROM_ISR(hiprio_woken);
}


void if_uart_init(void)
{
    uart_init(IF_UART_ID, BAUD_RATE);

    gpio_set_function(IF_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(IF_UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(IF_UART_ID, false, false);
    uart_set_format(IF_UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(IF_UART_ID, false);

    irq_set_exclusive_handler(IF_UART_IRQ, if_uart_irq_handler);
    irq_set_enabled(IF_UART_IRQ, true);
    uart_set_irq_enables(IF_UART_ID, true, false);

    tx_buf = xStreamBufferCreate(TXRX_BUF_SIZE, 1);
    rx_buf = xStreamBufferCreate(TXRX_BUF_SIZE, 1);

    xTaskCreate(if_uart_task,
                "if_uart",
                1024 * 4,
                NULL,
                IF_UART_TASK_PRIORITY,
                &if_uart_task_handle);

}


static void dispatch_messages(void)
{
    uint8_t buf[TXRX_BUF_SIZE];
    uint8_t data_char;
    uint32_t txlen;

    txlen = xStreamBufferReceive(tx_buf, buf, sizeof(buf), 1);
    if (txlen)
    {
        taskENTER_CRITICAL();
        uart_write_blocking(IF_UART_ID, buf, txlen);
        taskEXIT_CRITICAL();
    }
}


static void if_uart_task(void *params)
{
    for (;;)
    {
        dispatch_messages();
        vTaskDelay(10);
    }
}


void if_uart_tx(const uint8_t *buf, uint32_t buflen)
{
    xStreamBufferSend(tx_buf, buf, buflen, 1);
}


uint32_t if_uart_rx(uint8_t *buf, uint32_t buflen)
{
    return xStreamBufferReceive(rx_buf, buf, buflen, 1);
}
