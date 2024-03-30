import re

font_data = ''

with open('font5x7.h', 'r') as ff:
    for line in ff:
        mt = re.match(r'(\s*0x[0-9A-F]{2},){5}', line)
        if mt:
            cbts = mt.group(0).strip().split(',')
            cbts = [int(cb, 16) for cb in cbts if cb]
            t_cbts = []
            for i in range(7):
                tb = 0
                for j in range(5):
                    tb |= ((cbts[j] >> i) & 0x01) << j
                t_cbts.append(tb)
            bstr = ', '.join([f'0x{tb:02X}' for tb in t_cbts]) + ','
            font_data += '    ' + bstr + '\n'

TEMPLATE = \
'''
#ifndef __FONT5X7_TRANSPOSED_H
#define __FONT5X7_TRANSPOSED_H

/*  
 *  This is a transposed version of font5x7.h where the columns and rows are 
 *   swapped in each character bitmap, so each byte in this array reperesents a
 *   row, not a column.
 * 
 *  Extended 5x7 font defines ascii characters 0x20-0x7F (32-127) and includes
 *  Cyrillic characters coressponding to Windows-1251 encoding.
 *  This table defines 256 characters in total. 
 */

#define FONT_BYTES_IN_CHAR 7

static const unsigned char font5x7[] = {{
{0}
}};

#endif
'''

contents = TEMPLATE.format(font_data)
with open('font5x7_transposed.h', 'w') as off:
    off.write(contents)
