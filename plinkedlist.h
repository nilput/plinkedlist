#ifndef POOL_LINKEDLIST_H
#define POOL_LINKEDLIST_H
#include <stdint.h>
#include <string.h>
#include "util.h"


// 0 is a special index, it is treated like null
typedef int node_idx;

struct pll_node {
    int value;
    node_idx next;
};

struct pll_list {
    struct pll_node *data;
    struct bitset bitset;
    size_t len;
    size_t cap;
    size_t all_1_to;
};

static void pll_list_init(struct pll_list *list)
{
    list->len = 1; //because of null
    list->cap = 16;
    list->data = xmalloc(list->cap * sizeof(struct pll_node));

    list->all_1_to = 0;
    bitset_init(&list->bitset, list->cap);
    bitset_set_bit(&list->bitset, 0, 1); // set our null as occupied
}

static void pll_list_deinit(struct pll_list *list)
{
    bitset_deinit(&list->bitset);
    xfree(list->data);
    memset(list, 0, sizeof *list);
}

static node_idx pll_node_alloc(struct pll_list *list) 
{
    if (list->len == list->cap) {
        list->cap *= 2;
        list->data = xrealloc(list->data, list->cap * sizeof(struct pll_node));
        bitset_realloc(&list->bitset, list->cap);
    }

    long free_idx = bitset_find_false_bit(&list->bitset, list->all_1_to);
    assert(free_idx != -1);
    bitset_set_bit(&list->bitset, free_idx, 1);
    list->len++;
    list->all_1_to = free_idx;
    return free_idx;
}

static void pll_node_free(struct pll_list *list, node_idx idx)
{
    if (!idx)
        return; //we cant free our 'null'
    list->all_1_to = idx < list->all_1_to ? idx : list->all_1_to;
    list->len--;
    bitset_set_bit(&list->bitset, idx, 0);
}

static struct pll_node *pll_list_get(struct pll_list *list, node_idx idx)
{
    assert(idx != 0);
    return list->data + idx;
}
#endif /* POOL_LINKEDLIST_H */
