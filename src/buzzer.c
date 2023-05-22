
#include <hardware/pio.h>

#include "buzzer.h"
#include "buzzer.pio.h"

#define BUZZER_PIN 25

void buzzer_init(void)
{
    gpio_init(BUZZER_PIN);

    mdl2416c_program_init(pio0, BUZZER_PIN);

    buzzer_beep(0.1, 80.0);
    buzzer_beep(0.1, 100.0);
    buzzer_beep(0.1, 120.0);
}


void buzzer_beep(float duration, float freq)
{ 
    buzzer_beep_raw((uint32_t)(freq * duration),
                    (uint32_t)(1000000.0 / freq / 2.0));
}


void buzzer_beep_raw(uint32_t duration, uint32_t cycles_hi)
{
    pio_sm_put(pio0, BUZZER_PIO_SM, duration);     
    pio_sm_put(pio0, BUZZER_PIO_SM, cycles_hi);     
}