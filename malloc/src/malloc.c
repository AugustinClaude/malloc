#include "malloc.h"

#include "utils.h"

static struct block *mem = NULL;

static void *split_addr(struct block *block, size_t block_size, size_t aligned)
{
    if (block->size + sizeof(struct block) - block_size < sizeof(struct block))
    {
        block->state = 0;
        return shift_addr(block, sizeof(struct block));
    }

    struct block *new_free = shift_addr(block, block_size);
    new_free->size = block->size - block_size;
    new_free->state = 1;
    block->size = aligned;
    block->state = 0;

    new_free->prev = block;
    new_free->next = block->next;
    if (block->next)
        block->next->prev = new_free;
    block->next = new_free;

    return shift_addr(block, sizeof(struct block));
}

static struct block *init_mem(size_t page_aligned)
{
    mem = get_page(page_aligned);
    if (!mem)
        return NULL;

    mem->next = NULL;
    mem->prev = NULL;
    mem->size = page_aligned - sizeof(struct block);
    mem->state = 1;
    return mem;
}

static void *no_room(struct block *tmp, struct block *prev, size_t page_aligned,
                     size_t aligned)
{
    if (!tmp)
    {
        size_t block_size = aligned + sizeof(struct block);
        struct block *new_page = get_page(page_aligned);
        if (!new_page)
            return NULL;

        new_page->next = NULL;
        new_page->prev = prev;
        new_page->size = page_aligned - sizeof(struct block);
        new_page->state = 1;
        prev->next = new_page;

        if (new_page->size > aligned)
            return split_addr(new_page, block_size, aligned);
        else if (new_page->size == aligned)
        {
            new_page->state = 0;
            return shift_addr(new_page, sizeof(struct block));
        }
    }

    return NULL;
}

__attribute__((visibility("default"))) void *malloc(size_t size)
{
    if (size == 0)
        size++;

    size_t aligned = align(size, sizeof(long double));
    if (aligned == 0)
        return NULL;

    size_t block_size = aligned + sizeof(struct block);
    size_t page_aligned = align(block_size, get_pagesize());
    if (page_aligned == 0)
        return NULL;

    if (!mem)
    {
        mem = init_mem(page_aligned);
        if (!mem)
            return NULL;
    }

    struct block *prev = mem;
    struct block *tmp = mem;
    while (tmp)
    {
        if (tmp->state)
        {
            if (tmp->size > aligned)
                return split_addr(tmp, block_size, aligned);
            else if (tmp->size == aligned)
            {
                tmp->state = 0;
                return shift_addr(tmp, sizeof(struct block));
            }
        }

        prev = tmp;
        tmp = tmp->next;
    }

    return no_room(tmp, prev, page_aligned, aligned);
}

static struct block *merge_free(struct block *free)
{
    struct block *merged = free;

    if (free->prev && free->prev->state && in_same_page(free, free->prev))
    {
        free->prev->size += free->size + sizeof(struct block);
        free->prev->next = free->next;
        if (free->next)
            free->next->prev = free->prev;
        merged = free->prev;
    }

    if (merged->next && merged->next->state
        && in_same_page(merged, merged->next))
    {
        merged->size += merged->next->size + sizeof(struct block);
        merged->next = merged->next->next;
        if (merged->next)
            merged->next->prev = merged;
    }

    return merged;
}

void free_unmap(struct block *block, size_t block_size)
{
    size_t pagesize = get_pagesize();
    if (block_size % pagesize == 0 && page_begin(block, pagesize) == block)
    {
        if (!block->prev && !block->next)
            mem = NULL;
        if (block->prev)
            block->prev->next = block->next;
        else
            mem = block->next;

        if (block->next)
            block->next->prev = block->prev;

        munmap(block, block_size);
    }
}

static struct block *get_clangd(struct block *to_free, long free_ovrflw)
{
    struct block *new_free = shift_addr(to_free->next, -free_ovrflw);
    return new_free;
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (!ptr)
        return;

    struct block *to_free = shift_addr(ptr, -sizeof(struct block));
    to_free->state = 1;
    to_free = merge_free(to_free);

    size_t pagesize = get_pagesize();
    size_t free_block = to_free->size + sizeof(struct block);
    long free_ovrflw = free_block % pagesize;

    if (free_ovrflw != 0 && free_block > pagesize && to_free->next)
    {
        to_free->size -= free_ovrflw;
        if (free_ovrflw < (long)sizeof(struct block))
        {
            if (to_free->next->state)
            {
                struct block *new_free = get_clangd(to_free, free_ovrflw);
                memmove(new_free, to_free->next, sizeof(struct block));
                new_free->size += free_ovrflw;
                if (new_free->next)
                    new_free->next->prev = new_free;
                to_free->next = new_free;
                free_unmap(new_free, new_free->size + sizeof(struct block));
            }
        }
        else
        {
            struct block *new_free = shift_addr(to_free->next, -free_ovrflw);
            new_free->prev = to_free;
            new_free->next = to_free->next;
            new_free->size = free_ovrflw - sizeof(struct block);
            new_free->state = 1;
            to_free->next->prev = new_free;
            to_free->next = new_free;

            new_free = merge_free(new_free);
            free_unmap(new_free, new_free->size + sizeof(struct block));
        }
    }

    free_unmap(to_free, to_free->size + sizeof(struct block));
}

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);

    if (ptr && size == 0)
    {
        free(ptr);
        return NULL;
    }

    struct block *block = shift_addr(ptr, -sizeof(struct block));
    size_t aligned = align(size, sizeof(long double));
    size_t old_size = block->size;

    if (aligned == old_size)
        return ptr;

    void *new_ptr = malloc(size);
    if (!new_ptr)
        return NULL;

    size_t move_size = aligned < old_size ? aligned : old_size;
    memmove(new_ptr, ptr, move_size);
    free(ptr);
    return new_ptr;
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
    size_t res;
    if (__builtin_umull_overflow(nmemb, size, &res))
        return NULL;

    void *ptr = malloc(res);
    if (!ptr)
        return NULL;

    struct block *block = shift_addr(ptr, -sizeof(struct block));
    memset(ptr, 0, block->size);

    return ptr;
}
