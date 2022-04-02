// inspired by git implementation
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

#define FNV32_BASE 0x811c9dc5
#define FNV32_PRIME 0x01000193

unsigned int strhash(const char *str) {
    unsigned int c, hash = FNV32_BASE;
    while ((c = (unsigned char) *str++))
        hash = (hash * FNV32_PRIME) ^ c;
    return hash;
}

unsigned int memhash(const void *buf, size_t len) {
    unsigned int hash = FNV32_BASE;
    unsigned char *ucbuf = (unsigned char *) buf;
    while (len--) {
        unsigned int c = *ucbuf++;
        hash = (hash * FNV32_PRIME) ^ c;
    }
    return hash;
}

#define HASHMAP_INITIAL_SIZE 64
/* grow / shrink by 2^2 */
#define HASHMAP_RESIZE_BITS 2
/* load factor in percent */
#define HASHMAP_LOAD_FACTOR 80

static void alloc_table(struct hashmap *map, unsigned int size) {
    map->tablesize = size;
    map->table = calloc(size, sizeof(struct hashmap_entry *));
    map->grow_at = (unsigned int)((unsigned long)size * HASHMAP_LOAD_FACTOR / 100);
    if (size <= HASHMAP_INITIAL_SIZE)
        map->shrink_at = 0;
    else
        map->shrink_at = map->grow_at / ((1 << HASHMAP_RESIZE_BITS) + 1);
}

static inline int entry_equals(const struct hashmap *map, const struct hashmap_entry *e1, const struct hashmap_entry *e2) {
    return (e1 == e2) || (e1->hash == e2->hash && !map->cmpfn(e1, e2));
}

static inline unsigned int bucket(const struct hashmap *map, const struct hashmap_entry *key) {
    return key->hash & (map->tablesize - 1);
}

int hashmap_bucket(const struct hashmap *map, unsigned int hash) {
    return hash & (map->tablesize - 1);
}

static void rehash(struct hashmap *map, unsigned int newsize) {
    unsigned int i, oldsize = map->tablesize;
    struct hashmap_entry **oldtable = map->table;

    alloc_table(map, newsize);
    for (i = 0; i < oldsize; i++) {
        struct hashmap_entry *e = oldtable[i];
        while (e) {
            struct hashmap_entry *next = e->next;
            unsigned int b = bucket(map, e);
            e->next = map->table[b];
            map->table[b] = e;
            e = next;
        }
    }
    free(oldtable);
}

static inline struct hashmap_entry **find_entry_ptr(const struct hashmap *map, const struct hashmap_entry *key) {
    struct hashmap_entry **e = &map->table[bucket(map, key)];
    while (*e && !entry_equals(map, *e, key))
        e = &(*e)->next;
    return e;
}

int always_equal(const struct hashmap_entry *entry, const struct hashmap_entry *entry_or_key) {
    return 0;
}

void hashmap_init(struct hashmap *map, hashmap_cmp_fn equals_function, size_t initial_size) {
    unsigned int size = HASHMAP_INITIAL_SIZE;
    map->size = 0;
    map->cmpfn = equals_function ? equals_function : always_equal;
    initial_size = (unsigned int) ((unsigned long) initial_size * 100 / HASHMAP_LOAD_FACTOR);
    while (initial_size > size)
        size <<= HASHMAP_RESIZE_BITS;
    alloc_table(map, size);
}

void hashmap_free(struct hashmap *map, bool free_entries) {
    if (!map || !map->table)
        return;
    if (free_entries) {
        struct hashmap_iter iter;
        struct hashmap_entry *e;
        hashmap_iter_init(map, &iter);
        while ((e = hashmap_iter_next(&iter)))
            free(e);
    }
    free(map->table);
    memset(map, 0, sizeof(*map));
}

struct hashmap_entry *hashmap_get(const struct hashmap *map, const struct hashmap_entry *key) {
    return *find_entry_ptr(map, key);
}

struct hashmap_entry *hashmap_get_next(const struct hashmap *map, const struct hashmap_entry *entry) {
    struct hashmap_entry *e = entry->next;
    for (; e; e = e->next)
        if (entry_equals(map, entry, e))
            return e;
    return NULL;
}

void hashmap_add(struct hashmap *map, struct hashmap_entry *entry)
{
    unsigned int b = bucket(map, entry);

    /* add entry */
    entry->next = map->table[b];
    map->table[b] = entry;

    /* fix size and rehash if appropriate */
    map->size++;
    if (map->size > map->grow_at)
        rehash(map, map->tablesize << HASHMAP_RESIZE_BITS);
}

struct hashmap_entry *hashmap_remove(struct hashmap *map, const struct hashmap_entry *key) {
    struct hashmap_entry *old;
    struct hashmap_entry **e = find_entry_ptr(map, key);
    if (!*e)
        return NULL;

    /* remove existing entry */
    old = *e;
    *e = old->next;
    old->next = NULL;

    /* fix size and rehash if appropriate */
    map->size--;
    if (map->size < map->shrink_at)
        rehash(map, map->tablesize >> HASHMAP_RESIZE_BITS);
    return old;
}

struct hashmap_entry *hashmap_put(struct hashmap *map, struct hashmap_entry *entry) {
    struct hashmap_entry *old = hashmap_remove(map, entry);
    hashmap_add(map, entry);
    return old;
}

void hashmap_iter_init(struct hashmap *map, struct hashmap_iter *iter) {
    iter->map = map;
    iter->tablepos = 0;
    iter->next = NULL;
}

struct hashmap_entry *hashmap_iter_next(struct hashmap_iter *iter) {
    struct hashmap_entry *current = iter->next;
    for (;;) {
        if (current) {
            iter->next = current->next;
            return current;
        }

        if (iter->tablepos >= iter->map->tablesize)
            return NULL;

        current = iter->map->table[iter->tablepos++];
    }
}
