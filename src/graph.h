#ifndef __GRAPH_H
#define __GRAPH_H

#include <stdint.h>

#ifndef MIN
#define MIN(a, b) ((b) > (a) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((b) > (a) ? (b) : (a))
#endif

#ifndef SIGN
#define SIGN(a) ((a) > 0 ? 1 : ((a) == 0 ? 0 : -1))
#endif

typedef struct
{
    uint16_t width;
    uint16_t height;
    const uint8_t *bits;

} bitmap_t;


void circle(int16_t xc, int16_t yc, uint16_t r);

void blit(const bitmap_t *bm, int16_t sx, int16_t sy,
          uint16_t bx, uint16_t by, uint16_t w, uint16_t h);


#endif /* __GRAPH_H */