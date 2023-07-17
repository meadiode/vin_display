#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>

#define BUZZER_EVENT_BUFFER_NONE 0
#define BUZZER_EVENT_BUFFER_DONE 1
#define BUZZER_EVENT_BUFFER_FULL 2

void buzzer_init(void);

bool buzzer_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay);

bool buzzer_get_update(uint8_t *event);

#endif /* __BUZZER_H */