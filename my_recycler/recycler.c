#include "recycler.h"

#include <stdlib.h>

struct recycler *recycler_create(size_t block_size, size_t total_size)
{
    if (block_size % sizeof(size_t) != 0)
        return NULL;

    if (block_size == 0 || total_size == 0)
        return NULL;

    if (total_size % block_size != 0)
        return NULL;

    struct recycler *r = malloc(sizeof(struct recycler));
    if (!r)
        return NULL;

    r->block_size = block_size;
    r->capacity = total_size;
    r->chunk = malloc(block_size * total_size);
    if (!r->chunk)
        return NULL;
    r->free = r->chunk;

    return r;
}

void recycler_destroy(struct recycler *r)
{
    if (!r)
        return;

    char *chunk = r->chunk;
    char *tmp = r->chunk;
    while (tmp < chunk + r->capacity)
    {
        recycler_free(r, tmp);
        tmp += r->block_size;
    }

    free(r->chunk);
    free(r);
}

void *recycler_allocate(struct recycler *r)
{
    if (!r || !r->free)
        return NULL;

    struct free_list *tmp = r->free;
    r->free = tmp->next;
    return tmp;
}

void recycler_free(struct recycler *r, void *block)
{
    if (!r || !block)
        return;

    char *tmp = r->chunk;
    while (tmp != block)
        tmp += r->block_size;

    struct free_list *head = r->free;
    void *free = tmp;
    struct free_list *new_free = free;
    r->free = new_free;
    new_free->next = head;
}
