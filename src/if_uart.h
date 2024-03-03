#ifndef __IF_UART_H
#define __IF_UART_H

#include <stdint.h>


void if_uart_init(void);

void if_uart_tx(const uint8_t *buf, uint32_t buflen);
uint32_t if_uart_rx(uint8_t *buf, uint32_t buflen);

#endif /* __IF_UART_H */