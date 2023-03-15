
#include <string.h>
#include <hardware/pio.h>

#include "mdl2416c.h"
#include "mdl2416c.pio.h"


#define MDL_DATA_START_PIN     2
#define MDL_CHAR_POS_START_PIN 16

#define MDL_BL_PIN    9
#define MDL_WR_PIN   18
#define MDL_CU_PIN   19
#define MDL_CUE_PIN  20
#define MDL_CLR_PIN  21
#define MDL_CE_PIN   22


void mdl2416c_init(void)
{
    gpio_init(MDL_BL_PIN);
    gpio_init(MDL_CU_PIN);
    gpio_init(MDL_CUE_PIN);
    gpio_init(MDL_CLR_PIN);
    gpio_init(MDL_CE_PIN);

    gpio_set_dir(MDL_BL_PIN, GPIO_OUT);
    gpio_set_dir(MDL_CU_PIN, GPIO_OUT);
    gpio_set_dir(MDL_CUE_PIN, GPIO_OUT);
    gpio_set_dir(MDL_CLR_PIN, GPIO_OUT);
    gpio_set_dir(MDL_CE_PIN, GPIO_OUT);

    gpio_put(MDL_CU_PIN, 1);
    gpio_put(MDL_CUE_PIN, 0);
    gpio_put(MDL_CLR_PIN, 1);
    gpio_put(MDL_BL_PIN, 1);

    gpio_put(MDL_CE_PIN, 0);

    // PIO pio = pio0;
    // uint offset1 = pio_add_program(pio, &display_char_program);
    // uint offset2 = pio_add_program(pio, &set_char_pos_program);

    // mdl2416c_program_init(pio, 0, 1, offset1, offset2,
    //                       MDL_DATA_START_PIN,
    //                       MDL_CHAR_POS_START_PIN,
    //                       MDL_WR_PIN);


    // const char *words[] = {"COAX", "PINT", "PUNK", "SOCK", "PEAK",
    //                        "WELD", "LUSH", "FIZZ", "CLIP", "CLAM",
    //                        "GLUM", "GUSH", "CURL", "COZY", "YOGA",
    //                        "ROIL", "WISP", "MATH", "NOSH", "NOOK",
    //                        "GLOB", "HUFF", "PUFF", "ZEST", "WARY",
    //                        "DRIP", "DUNK", "JAZZ", "JIVE", "CORK",
    //                        "RASP", "RAZE", "QUID", "QUIT", "LYNX",
    //                        "LUCK", "BOLT", "HYPE", "HOAX", "WHAM"};

    // for (uint32_t i = 0; i < 10000; i++)
    // {
    //     for (uint32_t j = 0; j < sizeof(words) / sizeof(uint8_t*); j++)
    //     {
    //         const uint8_t *w = words[j];

    //         pio_sm_put_blocking(pio, 0, 0);
    //         sleep_ms(500);

    //         pio_sm_put_blocking(pio, 0, w[0] << 24 | w[1] << 16 | w[2] << 8 | w[3]);
    //         sleep_ms(500);            
    //     }
    // }
}


void mdl2416c_print(const uint8_t *str)
{
    uint8_t buf[4];
    uint8_t pos = 0;
    size_t len = strnlen(str, 40);
    uint32_t i;

/* TODO: Multiple consequent displays */

    for (i = 0; i == 0 || i < len; i += 4)
    {
        uint8_t nchars = MIN(sizeof(buf), len - i);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, str + i, nchars);

        for (uint8_t j = 0; j < sizeof(buf); j++)
        {
            uint8_t c = buf[j];
            if (c >= 'a' && c <= 'z')
            {
                buf[j] = c - ('a' - 'A');
            }
        }

        // pio_sm_put(pio0, 0, buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);
        // pio_sm_put(pio0, 1, nchars);
    }
}

