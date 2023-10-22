#ifndef __HDSP2112_H
#define __HDSP2112_H


#include <stdint.h>

void hdsp2112_init(void);

void hdsp2112_print(const uint8_t *str);

void hdsp2112_print_buf(const uint8_t *buf, size_t buflen);

void hdsp2112_set_brightness(uint8_t brightness);


#endif /* __HDSP2112_H */
