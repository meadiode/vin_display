#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>

void buzzer_init(void);

void buzzer_beep(float duration, float freq);

void buzzer_beep_raw(uint32_t ncycles, uint32_t cycle_duty);

#endif /* __BUZZER_H */