#ifndef __MACRO_H__
#define __MACRO_H__

#include <stddef.h>

#include "token.h"
#include "../common/hashmap.h"

enum macro_type {
    MACRO_TYPE__OBJECT = 1,
    MACRO_TYPE__FUNCTION,
};

struct macro_object {
    char *name;
    struct pp_token_list replacement_list;
    char *buf;
};

struct macro_function {
    char *name;
    struct pp_token_list replacement_list;
    char *buf;
    struct {
        unsigned int pos, len;
    } *params;
    size_t params_num;
    bool varargs;
};

struct macro_common {
    char *name;
    struct pp_token_list replacement_list;
    char *buf;
};

struct macro_entry {
    struct hashmap_entry entry;
    enum macro_type type;
    union {
        const char *name;
        struct macro_common common;
        struct macro_object object;
        struct macro_function function;
    } macro;
};

void macro_table_init(struct hashmap *macro_table);

void macro_entry_init(struct macro_entry *entry, char *name);

struct macro_entry *macro_table_get(const struct hashmap *macro_table, const char *name);

static inline void macro_table_add(struct hashmap *macro_table, struct macro_entry *entry) {
    hashmap_add(macro_table, &entry->entry);
}

struct macro_entry *macro_table_remove(struct hashmap *macro_table, const char *name);

void macro_entry_free(struct macro_entry *macro);

void macro_sanitize_replacement_list(struct macro_entry *macro);

bool are_macros_identical(const struct macro_entry *macro1, const struct macro_entry *macro2);

bool is_macro_name_reserved(const char *name);

#endif // __MACRO_H__
