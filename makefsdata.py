#! /usr/bin/python3

'''
Convert all files in a selected directory to hex data values that are put in
 a single .c file.
'''

import os, argparse


GEN_INFO = \
'''
/**
 *     This file was generated with makefsdata.py, do not edit manually.
 */
'''


def hexify_file(fname):
    lines = []
    size = 0

    with open(fname, 'rb') as file:
        while True:
            data = file.read(8)
            if not data:
                break
            size += len(data)
            line = ' '.join([f'0x{b:02x},' for b in data])
            line = '    ' + line

            lines.append(line)

    return size, lines


def make_fs_data(dir_, output_file):
    name = os.path.split(output_file)[-1]

    guard = f'__{name.upper()}_H'

    h_lines = [
        f'#ifndef {guard}',
        f'#define {guard}',
    ]

    h_lines += GEN_INFO.split('\n')

    h_lines += [
        f'#include <stdint.h>',
        f'#include <stddef.h>',
        f'',
        f'typedef struct',
        f'{{',
        f'    uint32_t size;',
        f'    const uint8_t *file_name;',
        f'    const uint8_t *file_contents;',
        f'',
        f'}} t_{name}_entry;',
        f'',
        f'',
        f'extern const t_{name}_entry {name}_list[];',
        f'extern const size_t {name}_list_size;',
        f'',
        f'#endif /* {guard} */',
    ]

    c_lines = GEN_INFO.split('\n')

    c_lines += [
        f'#include "{name}.h"',
        f'',
    ]

    entries = []

    for path, _, files in os.walk(dir_):
        npath = '/'.join(os.path.normpath(path).split('/')[1:])
        prefix = npath.replace('/', '_').upper()

        if prefix:
            prefix += '_'

        for file in files:
            size, lines = hexify_file(os.path.join(path, file))

            f_name_base = file.replace(".", "_").upper()

            f_name = f'{name.upper()}_{prefix}{f_name_base}'
            f_cont = f'{name.upper()}_{prefix}{f_name_base}_CONTENTS'
            f_size = f'{name.upper()}_{prefix}{f_name_base}_SIZE'


            c_lines += [
                f'const uint8_t {f_name}[] = '
                    f'"{os.path.join(npath, file)}";',
                f'const uint32_t {f_size} = {size}u;',
                f'const uint8_t {f_cont}[] = ',
            ]

            if lines:
                c_lines += [f'{{',] + lines + [f'}};']
            else:
                c_lines[-1] += f'{{}};'

            c_lines += [f'',]
            entries.append((f_size, f_name, f_cont))


    c_lines += [
        f'const t_{name}_entry {name}_list[] =',
        f'{{',
    ]

    for e in entries:
        c_lines += [f'    {{{e[0]}, {e[1]}, {e[2]}}},',]

    c_lines += [
        f'}};',
        f'',
        f'const size_t {name}_list_size = '
            f'sizeof({name}_list) / sizeof(t_{name}_entry);',
        f''
    ]

    with open(f'{output_file}.h', 'w') as h_file:
        h_file.write('\n'.join(h_lines))

    with open(f'{output_file}.c', 'w') as c_file:
        c_file.write('\n'.join(c_lines))

    print('Done!')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('target_dir', type=str,
                        help='Directory, files in which to be converted')
    parser.add_argument('-o', '--output', type=str, default='fsdata',
                        help='Output .c/.h file name (without extension)')
    args = parser.parse_args()

    make_fs_data(args.target_dir, args.output)
