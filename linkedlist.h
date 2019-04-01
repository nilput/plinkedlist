#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <stdint.h>
#include <string.h>
#include "util.h"

struct ll_node {
    int value;
    struct ll_node *next;
};

static struct ll_node* ll_node_alloc() 
{
    return xmalloc(sizeof(struct ll_node));
}

static void ll_node_free(struct ll_node *node)
{
    xfree(node);
}

#endif /* LINKEDLIST_H */
