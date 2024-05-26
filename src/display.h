#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(VIN_DISP_BOARD)
# error "Board variant is not defined"
#endif

#if (VIN_DISP_BOARD == BOARD_MDL2416C_X10)

# include "mdl2416c.h"
# define display_driver_init mdl2416c_init
# define display_print_buf mdl2416c_print_buf
# define display_set_brightness mdl2416c_set_brightness

#elif (VIN_DISP_BOARD == BOARD_HDSP2112_X6)

# include "hdsp2112.h"
# define display_driver_init hdsp2112_init
# define display_print_buf hdsp2112_print_buf
# define display_set_brightness hdsp2112_set_brightness

#elif (VIN_DISP_BOARD == BOARD_VQC10_X10)

# include "vqc10.h"
# define display_driver_init vqc10_init
# define display_print_buf vqc10_print_buf
# define display_set_brightness vqc10_set_brightness

#elif (VIN_DISP_BOARD == BOARD_HDSP2000_X20)

# include "hdsp2000.h"
# define display_driver_init hdsp2000_init
# define display_print_buf hdsp2000_print_buf
# define display_set_brightness hdsp2000_set_brightness
# define display_width hdsp2000_width
# define display_height hdsp2000_height
# define display_put_pixel hdsp2000_put_pixel
# define display_clear hdsp2000_clear
// # define display_swap_buffer hdsp2000_swap_buffer

#endif


void display_init(void);

bool display_send(const uint8_t *msg, uint32_t msg_len, uint32_t max_delay);

void display_swap_buffer(void);

void display_buffer_draw_finished_irq(void);

bool display_swap_buffer_requested(void);



#endif /* __DISPLAY_H */