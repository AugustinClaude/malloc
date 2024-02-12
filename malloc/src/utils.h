#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

void *page_begin(void *ptr, size_t page_size);
size_t align(size_t size, size_t alignment);
size_t get_pagesize(void);
void *get_page(size_t size);
void *shift_addr(void *block, long size);
int in_same_page(void *b1, void *b2);

#endif /* ! UTILS_H */
