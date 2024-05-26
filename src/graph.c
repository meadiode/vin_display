
#include "graph.h"
#include "display.h"


static void symplo8(int16_t xc, int16_t yc, int16_t x, int16_t y)  
{  
    display_put_pixel( x + xc, y + yc, 1);  
    display_put_pixel( x + xc,-y + yc, 1);  
    display_put_pixel(-x + xc,-y + yc, 1);  
    display_put_pixel(-x + xc, y + yc, 1);  
    display_put_pixel( y + xc, x + yc, 1);  
    display_put_pixel( y + xc,-x + yc, 1);  
    display_put_pixel(-y + xc,-x + yc, 1);  
    display_put_pixel(-y + xc, x + yc, 1);  
}  


void circle(int16_t xc, int16_t yc, uint16_t r)  
{  
    int16_t x = 0, y = r, d = 3 - (2 * r);  
    symplo8(xc, yc, x, y);  

    while (x <= y)  
    {  
        if (d <= 0)  
        {  
            d = d + (4 * x) + 6;  
        }  
        else  
        {  
            d = d + (4 * x) - (4 * y) + 10;  
            y = y - 1;  
        }  
        x = x + 1;  
        symplo8(xc, yc, x, y);  
    }
}


void blit(const bitmap_t *bm, int16_t sx, int16_t sy,
          uint16_t bx, uint16_t by, uint16_t w, uint16_t h)
{
    uint16_t x, y;
    uint16_t stride = bm->width / 8 + (bm->width % 8 ? 1 : 0);

    w = MIN(w, bm->width - bx);
    h = MIN(h, bm->height - by);

    for (x = 0; x < w; x++)
    {
        for (y = 0; y < h; y++)
        {
            uint16_t idx = (by + y) * stride + (bx + x) / 8;
            uint8_t bit = (bm->bits[idx] >> ((bx + x) % 8)) & 0x01;
            display_put_pixel(sx + x, sy + y, bit);
        }
    }
}

