#ifndef __VQC10_H
#define __VQC10_H


#include <stdint.h>

void vqc10_init(void);

void vqc10_print(const uint8_t *str);

void vqc10_print_buf(const uint8_t *buf, size_t buflen);

void vqc10_set_brightness(uint8_t brightness);

#endif /* __VQC10_H */