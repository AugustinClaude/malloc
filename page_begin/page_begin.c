#include "page_begin.h"

size_t addr_cast(void *ptr)
{
    return (size_t)ptr;
}

void *page_begin(void *ptr, size_t page_size)
{
    char *p = ptr;
    size_t start = (addr_cast(ptr) & ~(page_size - 1));
    char *offset = p - start;
    return p - addr_cast(offset);
}
