#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>

void buzzer_init(void);

void buzzer_beep(float duration, float freq);

void buzzer_beep_raw(uint32_t duration, uint32_t cycles_hi);

#endif /* __BUZZER_H */