#ifndef __MDL2416C_H
#define __MDL2416C_H

#include <stdint.h>

void mdl2416c_init(void);

void mdl2416c_print(const uint8_t *str);

void mdl2416c_print_buf(const uint8_t *buf, size_t buflen);


#endif /* __MDL2416C_H */