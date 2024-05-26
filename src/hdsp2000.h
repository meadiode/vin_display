#ifndef __HDSP2000_H
#define __HDSP2000_H


#include <stdint.h>

void hdsp2000_init(void);

void hdsp2000_print(const uint8_t *str);

void hdsp2000_print_buf(const uint8_t *buf, size_t buflen);

void hdsp2000_set_brightness(uint8_t brightness);

void hdsp2000_put_pixel(int16_t x, int16_t y, uint8_t val);

void hdsp2000_clear(void);

uint16_t hdsp2000_width(void);

uint16_t hdsp2000_height(void);

void hdsp2000_swap_buffer(void);


#endif /* __HDSP2000_H */
