#include "alignment.h"

size_t align(size_t size)
{
    size_t aligned = size / sizeof(long double);
    if (size % sizeof(long double) != 0)
        aligned++;

    size_t res;
    if (__builtin_umull_overflow(aligned, sizeof(long double), &res))
        return 0;
    return res;
}
