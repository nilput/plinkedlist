#ifndef UTIL_H
#define UTIL_H
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
void die(const char *msg);
void *xmalloc(size_t sz);
void *xrealloc(void *m, size_t sz);
void *xfree(void *m);

struct bitset {
    unsigned *data;
    size_t bit_len;
};
/*zero based indices*/
/*all bits are set to 0 during both initialization and new bits during reallocation*/
void bitset_init(struct bitset *bitset, size_t bit_len);
void bitset_realloc(struct bitset *bitset, size_t bit_len);
void bitset_deinit(struct bitset *bitset);
bool bitset_get_bit(struct bitset *bitset, size_t bit_idx);
void bitset_set_bit(struct bitset *bitset, size_t bit_idx, bool state);
long bitset_find_true_bit(struct bitset *bitset,  size_t start_at_bit_idx);
long bitset_find_false_bit(struct bitset *bitset,  size_t start_at_bit_idx);


bool argv_get_int(int argc, const char **argv, const char *key, int *out_val, int default_val);

#endif /*UTIL_H*/
