#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define TEMP_ADC_INPUT  4


void display_init(void);

bool display_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay);


#endif /* __DISPLAY_H */