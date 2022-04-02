#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "macro.h"

static int macro_entry_cmp(const struct hashmap_entry *entry, const struct hashmap_entry *entry_or_key) {
    const struct macro_entry *a = (const struct macro_entry *)entry;
    const struct macro_entry *b = (const struct macro_entry *)entry_or_key;
    return strcmp(a->macro.common.name, b->macro.common.name);
}

void macro_table_init(struct hashmap *macro_table) {
    hashmap_init(macro_table, &macro_entry_cmp, 0);
}

void macro_entry_init(struct macro_entry *entry, char *name) {
    unsigned int hash = strhash(name);
    entry->macro.common.name = name;
    hashmap_entry_init(&entry->entry, hash);
}

static void macro_key_init(struct macro_entry *entry, const char *name) {
    unsigned int hash = strhash(name);
    entry->macro.name = name;
    hashmap_entry_init(&entry->entry, hash);
}

struct macro_entry *macro_table_get(const struct hashmap *macro_table, const char *name) {
    struct macro_entry key;
    macro_key_init(&key, name);
    return (struct macro_entry *)hashmap_get(macro_table, &key.entry);
}

struct macro_entry *macro_table_remove(struct hashmap *macro_table, const char *name) {
    struct macro_entry key;
    macro_key_init(&key, name);
    return (struct macro_entry *)hashmap_remove(macro_table, &key.entry);
}

void macro_entry_free(struct macro_entry *macro) {
    struct pp_token *head;
    free(macro->macro.common.name);
    macro->macro.common.name = NULL;
    macro->entry.hash = 0;
    free(macro->macro.common.buf);
    macro->macro.common.buf = NULL;
    head = macro->macro.common.replacement_list.head;
    while (head != NULL) {
        struct pp_token *next = head->next;
        free(head);
        head = next;
    }
    macro->macro.common.replacement_list.head = NULL;
    if (macro->type == MACRO_TYPE__FUNCTION) {
        free(macro->macro.function.params);
        macro->macro.function.params_num = 0;
        macro->macro.function.varargs = false;
    }
}

void macro_sanitize_replacement_list(struct macro_entry *macro) {
    struct pp_token *head = macro->macro.common.replacement_list.head;
    struct pp_token *prev = NULL, *prev2 = NULL;
    struct pp_token *pp_token;
    bool whitespace = false;
    for (pp_token = head; pp_token != NULL; pp_token = pp_token->next) {
        if (pp_token->type == __WHITESPACE ||
            pp_token->type == __COMMENT_OLD_STYLE ||
            pp_token->type == __COMMENT_NEW_STYLE) {
            pp_token->type == __WHITESPACE;
            if (whitespace) {
                assert(prev->next == pp_token);
                assert(prev->pos + prev->len == pp_token->pos);
                prev->len += pp_token->len;
                prev->next = pp_token->next;
                free(pp_token);
                pp_token = prev;
            } else {
                prev = pp_token;
                pp_token->type = __WHITESPACE;
                whitespace = true;
            }
        } else {
            whitespace = false;
            prev2 = pp_token;
        }
    }
    if (head != NULL && head->type == __WHITESPACE) {
        macro->macro.common.replacement_list.head = head->next;
        free(head);
    }
    if (prev != NULL && prev->next == NULL) {
        assert(prev2 != NULL && prev2->next == prev);
        prev2->next = NULL;
        free(prev);
    }
}

// we already assume they have the same name
bool are_macros_identical(const struct macro_entry *macro1, const struct macro_entry *macro2) {
    enum macro_type type = macro1->type;
    struct pp_token *p1, *p2;
    const char *buf1 = macro1->macro.common.buf, *buf2 = macro2->macro.common.buf;
    if (type != macro2->type)
        return false;
    p1 = macro1->macro.common.replacement_list.head;
    p2 = macro2->macro.common.replacement_list.head;
    for (; p1 && p2; p1 = p1->next, p2 = p2->next) {
        unsigned long len = p1->len;
        enum pp_token_type type = p1->type;
        if (type != p2->type)
            return false;
        // whitespace seperations are considered identical
        if (type != __WHITESPACE &&
            (len != p2->len ||
            memcmp(buf1 + p1->pos, buf2 + p2->pos, len)))
            return false;
    }
    if (p1 || p2)
        return false;
    if (type == MACRO_TYPE__FUNCTION) {
        size_t params_num = macro1->macro.function.params_num;
        size_t i;
        if (params_num != macro2->macro.function.params_num)
            return false;
        if (macro1->macro.function.varargs != macro2->macro.function.varargs)
            return false;
        for (i = 0; i < params_num; ++i) {
            unsigned int len = macro1->macro.function.params[i].len;
            unsigned int pos1 = macro1->macro.function.params[i].pos;
            unsigned int pos2 = macro2->macro.function.params[i].pos;
            if (len != macro2->macro.function.params->len)
                return false;
            if (memcmp(buf1 + pos1, buf2 + pos2, len))
                return false;
        }
    }
    return true;
}

bool is_macro_name_reserved(const char *name) {
    return !strcmp(name, "defined") ||
            !strcmp(name, "__DATE__") ||
            !strcmp(name, "__FILE__") ||
            !strcmp(name, "__LINE__") ||
            !strcmp(name, "__STDC__") ||
            !strcmp(name, "__STDC_HOSTED__") ||
            !strcmp(name, "__STDC_MB_MIGHT_NEQ_WC__") ||
            !strcmp(name, "__STDC_VERSION__") ||
            !strcmp(name, "__TIME__") ||
            !strcmp(name, "__STDC_IEC_559__") ||
            !strcmp(name, "__STDC_IEC_559_COMPLEX__") ||
            !strcmp(name, "__STDC_ISO_10646__");
}
