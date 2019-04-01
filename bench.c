#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"
#include "plinkedlist.h"
#include "util.h"
#include "bench_util.h"
#include "timer.h"


#define N_HEADS_NODES 1000
//higher number: less chance
#define REPLACE_CHANCE 128
#define DELETE_CHANCE 8
#define RUIN_CHANCE   32
#define MAX_HEAP_PTRS 20000


static struct opts {
    int help;
    int n_iters;
    int enable_ll;
    int enable_pll;
    int ruin_heap;
} opts;

struct stats {
    long n_alloc;
    long n_dealloc;
    long n_heads_replace;
    long n_ruin_heap;
    double checksum_time;
    double insert_time;
    double delete_time;
    double exc_alloc_time;
};

static struct stats pll_stats = {0};
static struct stats ll_stats = {0};

static struct timer_info tinfo;

void dump_stats(const char *name, struct stats *st)
{
    double total_time = st->checksum_time + st->insert_time + st->delete_time;
    printf(
"stats for %s:\n"
"\tn_alloc:         %ld\n"
"\tn_dealloc:       %ld\n"
"\tn_heads_replace: %ld\n"
"\tn_ruin_heap:     %ld\n"
"\ttotal_time:      %.3f\n"
"\tinsert_time:     %.3f\n"
"\tchecksum_time:   %.3f\n"
"\tdelete_time:     %.3f\n"
"\texc_alloc_time:  %.3f\n",
    name,
    st->n_alloc,
    st->n_dealloc,
    st->n_heads_replace,
    st->n_ruin_heap,
    total_time,
    st->insert_time,
    st->checksum_time,
    st->delete_time,
    st->exc_alloc_time); //printf
}

struct heap_info {
    int n_ptrs;
    void *heap_ptrs[MAX_HEAP_PTRS];
};
static struct heap_info pll_hinfo = {0};
static struct heap_info ll_hinfo = {0};

bool ruin_heap(struct heap_info *h) {
    if (h->n_ptrs >= MAX_HEAP_PTRS)
        return false;
    //maximum: 2^10 bytes, which is peanuts
    for (int i=0; (i < 10) && (h->n_ptrs < MAX_HEAP_PTRS); i++) {
        if (rand()%2) {
            unsigned char *m = xmalloc(1U << i);
            m[0] = 0xFF;
            h->heap_ptrs[h->n_ptrs++] = m;
            if (h->n_ptrs == MAX_HEAP_PTRS)
                break;
        }
    }
    return true;
}
void unruin_heap(struct heap_info *h) {
    for (int i=h->n_ptrs-1; i>=0; i--) {
        xfree(h->heap_ptrs[i]);
        h->heap_ptrs[i] = NULL;
    }
    h->n_ptrs = 0;
}


//pool allocated linked list
static struct pll_list *pll = NULL;
static node_idx pll_root = 0;
static node_idx pll_heads_nodes[N_HEADS_NODES] = {0};
static int pll_value_num = 1;
//when deallocating we make sure we don't hold a dangling index (which is reclaimed by the list)
static void pll_heads_remove_if_exists(node_idx node) {
    struct pll_node *pll_node = pll_list_get(pll, node);
    int hash = pll_node->value % N_HEADS_NODES;
    if (pll_heads_nodes[hash] == node)
        pll_heads_nodes[hash] = 0;
}

//classic linked list
static struct ll_node *ll_root = NULL;
static struct ll_node *ll_heads_nodes[N_HEADS_NODES] = {NULL};
static int ll_value_num = 1;
//when deallocating we make sure we don't hold a dangling pointer
static void ll_heads_remove_if_exists(struct ll_node *ll_node) {
    int hash = ll_node->value % N_HEADS_NODES;
    if (ll_heads_nodes[hash] == ll_node)
        ll_heads_nodes[hash] = NULL;
}



/*support code for both variants*/

//iterates nodes keeping a checksum that is used to verify both variants are equivilant, also helps us test cache performance
static unsigned pll_iter_nodes_checksum() {
    unsigned checksum = 0;
    node_idx head = pll_root;
    while (head) {
        struct pll_node *node = pll_list_get(pll, head);
        checksum = update_adler32(checksum, (const unsigned char *)&node->value, sizeof(int));
        /* printf("node: %d, value: %d, next: %d\n", (int)head, node->value, (int)node->next); */
        head = node->next;
    }
    return checksum;
}
static unsigned ll_iter_nodes_checksum() {
    unsigned checksum = 0;
    struct ll_node *head = ll_root;
    while (head) {
        checksum = update_adler32(checksum, (const unsigned char *)&head->value, sizeof(int));
        /* printf("node: %d, value: %d, next: %d\n", (int)head, node->value, (int)node->next); */
        head = head->next;
    }
    return checksum;
}

static node_idx pll_insert(struct pll_list *list, node_idx at, int value) {
    struct timer_info tmp;
    timer_begin(&tmp);
    node_idx tail = pll_node_alloc(list);
    pll_stats.exc_alloc_time += timer_dt(&tmp);

    struct pll_node *at_node = pll_list_get(list, at);
    struct pll_node *tail_node = pll_list_get(list, tail);
    tail_node->next = at_node->next;
    tail_node->value = value;
    assert((tail != tail_node->next) && (tail != at));
    at_node->next = tail;
    return tail;
}
static struct ll_node *ll_insert(struct ll_node *node, int value) {
    struct timer_info tmp;
    timer_begin(&tmp);
    struct ll_node *tail_node = ll_node_alloc();
    ll_stats.exc_alloc_time += timer_dt(&tmp);


    tail_node->next = node->next;
    tail_node->value = value;
    node->next = tail_node;
    return tail_node;
}



void pll_random_inserts()
{
    srand(0xBEEF);
    for (int i=0; i<opts.n_iters; i++) {
        int rnd = rand();
        int insert_at_heads_idx = rnd % N_HEADS_NODES;
        node_idx node = pll_heads_nodes[insert_at_heads_idx];
        if (!node) {
            node = pll_root;
        }
        node_idx new_node = pll_insert(pll, node, pll_value_num);
        pll_stats.n_alloc++;
        if (rnd % REPLACE_CHANCE == 0) { 
            //add it to our heads list, replacing whatever was there
            pll_heads_nodes[pll_value_num % N_HEADS_NODES] = new_node;
            pll_stats.n_heads_replace++;
        }
        if (opts.ruin_heap && ((rnd % RUIN_CHANCE) == 0)) {
            if (ruin_heap(&pll_hinfo))
                pll_stats.n_ruin_heap++;
            else
                unruin_heap(&pll_hinfo);
        }
        pll_value_num++;
    }
}
void ll_random_inserts()
{
    srand(0xBEEF);
    for (int i=0; i<opts.n_iters; i++) {
        int rnd = rand();
        int insert_at_heads_idx = rnd % N_HEADS_NODES;

        struct ll_node *node = ll_heads_nodes[insert_at_heads_idx];
        if (!node) {
            node = ll_root;
        }
        struct ll_node *new_node = ll_insert(node, ll_value_num);
        ll_stats.n_alloc++;
        if (rnd % REPLACE_CHANCE == 0) {
            //add it to our heads list, replacing whatever was there
            ll_heads_nodes[ll_value_num % N_HEADS_NODES] = new_node;
            ll_stats.n_heads_replace++;
        }
        if (opts.ruin_heap && ((rnd % RUIN_CHANCE) == 0)) {
            if (ruin_heap(&ll_hinfo))
                ll_stats.n_ruin_heap++;
            else
                unruin_heap(&ll_hinfo);
        }
        ll_value_num++;
    }
}

void pll_random_deletes()
{
    srand(0xFEEDBEEF);
    for (int i=0; i<opts.n_iters; i++) {
        int rnd = rand();
        if (rnd % DELETE_CHANCE == 0) {
            int delete_at_heads_idx = rnd % N_HEADS_NODES;
            node_idx node = pll_heads_nodes[delete_at_heads_idx];
            if (!node) {
                continue;
            }
            struct pll_node *pll_node = pll_list_get(pll, node);
            node_idx next_node = pll_node->next;
            pll_stats.n_dealloc++;
            if (next_node) {
                struct pll_node *pll_next_node = pll_list_get(pll, next_node);
                pll_node->next = pll_next_node->next; //connect [node] [node.next] [node.next.next]
                                                      //          *->->->->->->->->->->^
                pll_heads_remove_if_exists(next_node);
                pll_node_free(pll, next_node);
            }
            else {
                pll_node->next = 0;
            }
        }
    }
}
void ll_random_deletes()
{
    srand(0xFEEDBEEF);
    for (int i=0; i<opts.n_iters; i++) {
        int rnd = rand();
        if (rnd % DELETE_CHANCE == 0) {
            int delete_at_heads_idx = rnd % N_HEADS_NODES;
            struct ll_node *node = ll_heads_nodes[delete_at_heads_idx];
            if (!node) {
                continue;
            }
            struct ll_node *next_node = node->next;
            if (next_node) {
                node->next = next_node->next; //connect [node] [node.next] [node.next.next]
                                              //          *->->->->->->->->->->^
                ll_heads_remove_if_exists(next_node);
                ll_node_free(next_node);
                ll_stats.n_dealloc++;
            }
            else {
                node->next = NULL;
            }
        }
    }
}


void ll_dealloc()
{
    struct ll_node *head = ll_root;
    while (head) {
        struct ll_node *tmp = head->next;
        ll_node_free(head);
        head = tmp;
    }
    ll_root = NULL;
}


void parse_argv(int argc, const char **argv)
{
    argv_get_int(argc, argv, "-n", &opts.n_iters, 500000);
    if (argv_get_int(argc, argv, "-h", &opts.help, 0)) opts.help = 1;
    if (argv_get_int(argc, argv, "--enable-pll", &opts.enable_pll, 0)) opts.enable_pll = 1;
    if (argv_get_int(argc, argv, "--enable-ll", &opts.enable_ll,  0)) opts.enable_ll = 1;
    if (argv_get_int(argc, argv, "--ruin-heap", &opts.ruin_heap, 0)) opts.ruin_heap = 1;
    if (opts.help) {
        printf(
        "Options:\n"
        "\t-n\tnumber of iterations.\n"
        "\t--enable-ll\tenable classic linked list\n"
        "\t--enable-pll\tenable pool allocated linked list\n"
        "\t--ruin-heap\tattempt to simulate heap fragmentation, \n"
        "\tthis actually makes things faster instead of the intended result (default: off)\n"
        ); //printf
        exit(0);
    }
    if (!opts.enable_ll && !opts.enable_pll) {
        opts.enable_ll = opts.enable_pll = 1;
    }
    printf("bench\tn_iters: %d, ruin_heap:%d\n", opts.n_iters, opts.ruin_heap);
}

static void do_inserts() {
    if (opts.enable_pll) {
        timer_begin(&tinfo);
        pll_random_inserts();
        pll_stats.insert_time += timer_dt(&tinfo);
    }
    if (opts.enable_ll) {
        timer_begin(&tinfo);
        ll_random_inserts();
        ll_stats.insert_time += timer_dt(&tinfo);
    }
}
static void do_deletes() {
    if (opts.enable_pll) {
        timer_begin(&tinfo);
        pll_random_deletes();
        pll_stats.delete_time += timer_dt(&tinfo);
    }
    if (opts.enable_ll) {
        timer_begin(&tinfo);
        ll_random_deletes();
        ll_stats.delete_time += timer_dt(&tinfo);
    }
}
static void do_checksums(int *pll_hash, int *ll_hash) {
    if (opts.enable_pll) {
        timer_begin(&tinfo);
        *pll_hash = pll_iter_nodes_checksum();
        printf("\tpll_hash: %u\n", *pll_hash);
        pll_stats.checksum_time += timer_dt(&tinfo);
    }
    if (opts.enable_ll) {
        timer_begin(&tinfo);
        *ll_hash = ll_iter_nodes_checksum();
        printf("\tll_hash:  %u\n", *ll_hash);
        ll_stats.checksum_time += timer_dt(&tinfo);
    }
}

int main(int argc, const char **argv)
{
    parse_argv(argc, argv);

    struct pll_list list;
    pll_list_init(&list);
    pll = &list; //global list variable

    if (opts.enable_pll) {
        pll_root = pll_node_alloc(pll); //global root variable
        struct pll_node *pll_root_node = pll_list_get(pll, pll_root);
        pll_root_node->value = 0;
        pll_root_node->next = 0;
    }

    if (opts.enable_ll) {
        ll_root = ll_node_alloc(); //global root variable
        ll_root->next = NULL;
        ll_root->value = 0;
    }


    do_inserts();

    unsigned pll_hash; 
    unsigned ll_hash; 

    puts("checksums 1:");
    do_checksums(&pll_hash, &ll_hash);

    if (opts.ruin_heap){
        unruin_heap(&pll_hinfo);
        unruin_heap(&ll_hinfo);
    }

    do_deletes();

    puts("checksums 2:");
    do_checksums(&pll_hash, &ll_hash);

    do_inserts();


    puts("checksums 3:");
    do_checksums(&pll_hash, &ll_hash);

    if (opts.enable_pll) {
        dump_stats("pool allocated linked list", &pll_stats);
        pll_list_deinit(&list);
        pll_root = 0;
    }
    if (opts.enable_ll) {
        dump_stats("classic linked list", &ll_stats);
        ll_dealloc();
        ll_root = NULL;
    }

    if (opts.ruin_heap){
        unruin_heap(&pll_hinfo);
        unruin_heap(&ll_hinfo);
    }
}
