/*
 * divmod10.c
 *
 * Created: 01.05.2016 21:48:52
 *  Author: dark
 */ 

#include <avr/io.h>
#include "divmod10.h"

typedef struct {
    uint8_t quot;
    uint8_t rem;
} divmod810_t;

inline static divmod810_t divmodu810(uint8_t n) {
    divmod810_t res;
    // multiply to 0.8
    res.quot = n >> 1;
    res.quot += res.quot >> 1;
    res.quot += res.quot >> 4;
    uint8_t qq = res.quot;
    // divide to 8
    res.quot >>= 3;
    // calculate module
    res.rem = (uint8_t)(n - ((res.quot << 1) + (qq & ~7)));
    // correction module
    if(res.rem > 9)
    {
        res.rem -= 10;
        res.quot++;
    }
    return res;
}

uint8_t* utod_fast_div8(uint8_t value, uint8_t* buffer) {
    for (uint8_t i = 0; i < 3; i++) {
        divmod810_t res = divmodu810(value);
        *(buffer + i) = res.rem;
        value = res.quot;
    }
    return buffer;
}

