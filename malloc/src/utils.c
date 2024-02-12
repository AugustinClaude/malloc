#include "utils.h"

static size_t addr_cast(void *ptr)
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

size_t align(size_t size, size_t alignment)
{
    size_t aligned = size / alignment;
    if (size % alignment != 0)
        aligned++;

    size_t res;
    if (__builtin_umull_overflow(aligned, alignment, &res))
        return 0;
    return res;
}

size_t get_pagesize(void)
{
    return sysconf(_SC_PAGESIZE);
}

void *get_page(size_t size)
{
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
        return NULL;
    return addr;
}

void *shift_addr(void *block, long size)
{
    char *tmp = block;
    tmp += size;
    return tmp;
}

int in_same_page(void *b1, void *b2)
{
    size_t pagesize = get_pagesize();
    void *begin1 = page_begin(b1, pagesize);
    void *begin2 = page_begin(b2, pagesize);

    return begin1 == begin2;
}
