#include "plinkedlist.h"
#include <stdio.h>

struct opts {
    int n_rand;
} opts;

void pll_iter_nodes(struct pll_list *list, node_idx root) {
    node_idx head = root;
    while (head) {
        struct pll_node *node = pll_list_get(list, head);
        printf("node: %d, value: %d, next: %d\n", (int)head, node->value, (int)node->next);
        head = node->next;
    }
}


/*
 *  at  ->  at.next
 *
 *  at  -> value -> at.next
 *
 *  returns the node_idx of the newly created node
 */
node_idx pll_insert(struct pll_list *list, node_idx at, int value) {
    node_idx tail = pll_node_alloc(list);
    struct pll_node *at_node = pll_list_get(list, at);
    struct pll_node *tail_node = pll_list_get(list, tail);
    tail_node->next = at_node->next;
    tail_node->value = value;
    at_node->next = tail;
    return tail;
}


int main(int argc, const char **argv) 
{
    struct pll_list list;
    pll_list_init(&list);

    //node_idx is a typedef of an int or something
    node_idx root = pll_node_alloc(&list);
    struct pll_node *root_node = pll_list_get(&list, root);
    root_node->value = 0;
    root_node->next = 0; //'null'

    puts("iterating nodes:");
    pll_iter_nodes(&list, root);

    node_idx tail = root;
    for (int i=0; i<3; i++) {
        tail = pll_insert(&list, tail, i+1);
    }

    puts("iterating nodes:");
    pll_iter_nodes(&list, root);

    srand(0);
    if (argv_get_int(argc, argv, "--rand", &opts.n_rand, 100)) {
        for (int i=0; i<opts.n_rand; i++) {
            node_idx at;
            if ((rand() % 2) == 0)
                at = root;
            else 
                at = tail;

            node_idx new_node = pll_insert(&list, at, i + 100);
            if (at == tail)
                tail = new_node; //update our tail
        }
        puts("iterating randomly inserted nodes:");
        pll_iter_nodes(&list, root);
    }

    pll_list_deinit(&list); //this deallocates everything
}
