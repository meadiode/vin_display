#ifndef __USB_SERIAL_H
#define __USB_SERIAL_H

#include <stdint.h>

extern uint8_t usb_serial_id[];

void usb_serial_init(void);

void usb_serial_tx(const uint8_t *buf, uint32_t buflen);
uint32_t usb_serial_rx(uint8_t *buf, uint32_t buflen);

#endif /* __USB_SERIAL_H */