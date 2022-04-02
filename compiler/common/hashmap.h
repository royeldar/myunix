// inspired by git implementation
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stddef.h>
#include <stdbool.h>

struct hashmap_entry {
    struct hashmap_entry *next;
    unsigned int hash;
};

typedef int (*hashmap_cmp_fn)(const struct hashmap_entry *entry, const struct hashmap_entry *entry_or_key);

struct hashmap {
    struct hashmap_entry **table;
    hashmap_cmp_fn cmpfn;
    unsigned int size;
    unsigned int tablesize;
    unsigned int grow_at;
    unsigned int shrink_at;
};

struct hashmap_iter {
    struct hashmap *map;
    struct hashmap_entry *next;
    unsigned int tablepos;
};

unsigned int strhash(const char *buf);
unsigned int memhash(const void *buf, size_t len);

void hashmap_init(struct hashmap *map, hashmap_cmp_fn equals_function, size_t initial_size);
void hashmap_free(struct hashmap *map, bool free_entries);

static inline void hashmap_entry_init(struct hashmap_entry *entry, unsigned int hash) {
    entry->hash = hash;
    entry->next = NULL;
}

struct hashmap_entry *hashmap_get(const struct hashmap *map, const struct hashmap_entry *key);
struct hashmap_entry *hashmap_get_next(const struct hashmap *map, const struct hashmap_entry *entry);
void hashmap_add(struct hashmap *map, struct hashmap_entry *entry);
struct hashmap_entry *hashmap_put(struct hashmap *map, struct hashmap_entry *entry);
struct hashmap_entry *hashmap_remove(struct hashmap *map, const struct hashmap_entry *key);

void hashmap_iter_init(struct hashmap *map, struct hashmap_iter *iter);
struct hashmap_entry *hashmap_iter_next(struct hashmap_iter *iter);
static inline struct hashmap_entry *hashmap_iter_first(struct hashmap *map, struct hashmap_iter *iter) {
    hashmap_iter_init(map, iter);
    return hashmap_iter_next(iter);
}

#endif // __HASHMAP_H__
