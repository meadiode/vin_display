#ifndef __FSDATA_H
#define __FSDATA_H

/**
 *     This file was generated with makefsdata.py, do not edit manually.
 */

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint32_t size;
    const uint8_t *file_name;
    const uint8_t *file_contents;

} t_fsdata_entry;


extern const t_fsdata_entry fsdata_list[];
extern const size_t fsdata_list_size;

#endif /* __FSDATA_H */