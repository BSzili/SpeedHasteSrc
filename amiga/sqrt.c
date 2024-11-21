#include <sqrt.h>

uint32 SQR_Sqrt(uint32 v)
{
    uint32 res = 0;
    uint32 bit = 1 << 30;

    while (bit > v) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (v >= res + bit) {
            v -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }

    return res;
}
