#include "allocator.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "utils.h"

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

struct blk_allocator *blka_new(void)
{
    struct blk_allocator *blka = malloc(sizeof(struct blk_allocator));
    blka->meta = NULL;

    return blka;
}

struct blk_meta *blka_alloc(struct blk_allocator *blka, size_t size)
{
    long page_size = sysconf(_SC_PAGESIZE);
    size_t aligned = align(size + sizeof(struct blk_meta), page_size);
    if (aligned == 0)
        return NULL;

    struct blk_meta *blk = mmap(NULL, aligned, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (blk == MAP_FAILED)
        return NULL;

    blk->next = blka->meta;
    blk->size = aligned - sizeof(struct blk_meta);

    blka->meta = blk;
    return blk;
}

void blka_free(struct blk_meta *blk)
{
    munmap(blk, blk->size + sizeof(struct blk_meta));
}

void blka_pop(struct blk_allocator *blka)
{
    if (!blka->meta)
        return;

    struct blk_meta *tmp = blka->meta;
    blka->meta = blka->meta->next;
    blka_free(tmp);
}

void blka_delete(struct blk_allocator *blka)
{
    while (blka->meta)
        blka_pop(blka);

    free(blka);
}
