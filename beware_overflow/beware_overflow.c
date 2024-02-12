#include "beware_overflow.h"

void *beware_overflow(void *ptr, size_t nmemb, size_t size)
{
    size_t res;
    int overflow = __builtin_umull_overflow(nmemb, size, &res);
    if (overflow)
        return NULL;

    char *p = ptr;
    p += res;
    return p;
}
