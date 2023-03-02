#ifndef __FPANEL_H
#define __FPANEL_H

#include <stdint.h>
#include <stdbool.h>


#define POWER_BTN_GPIO          22
#define RESET_BTN_GPIO          21
#define POWER_LED_SENSE_GPIO    27
#define HDD_LED_SENSE_GPIO      26

#define POWER_LED_SENSE_ADC_INPUT 1
#define HDD_LED_SENSE_ADC_INPUT   0


void fpanel_init(void);

bool fpanel_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay);

void fpanel_power_btn(bool down);
void fpanel_reset_btn(bool down);

#endif /* __FPANEL_H */