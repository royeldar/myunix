#ifndef __UCN_H__
#define __UCN_H__

#include <stdbool.h>

static inline bool is_ucn_valid(unsigned long ucn) {
    return ucn == 0x24 || ucn == 0x40 || ucn == 0x60 ||
            ucn >= 0xE000 || (ucn >= 0xA0 && ucn < 0xD800);
}

bool is_ucn_valid_identifier_initial(unsigned long ucn);

bool is_ucn_valid_identifier(unsigned long ucn);

#endif // __UCN_H__
