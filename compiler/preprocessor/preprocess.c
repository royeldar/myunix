#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "preprocess.h"
#include "translate.h"
#include "macro.h"

int _preprocess(const char *filename, FILE **dest, struct pp_token_list *pp_tokens, unsigned long **splice_pos, size_t *splice_count) {
    int err = 0;
    FILE *src;
    FILE *src1 = NULL;
    FILE *src2 = NULL;
    src = fopen(filename, "rb");
    if (src == NULL)
        goto error;
    src1 = tmpfile();
    if (src1 == NULL)
        goto error;
    if (sanitize_source(src, src1))
        goto error;
    fflush(src1);
    src2 = tmpfile();
    if (src2 == NULL)
        goto error;
    if (replace_trigraphs(src1, src2))
        goto error;
    fflush(src2);
    *dest = tmpfile();
    if (*dest == NULL)
        goto error;
    if (splice_lines(src2, *dest, splice_pos, splice_count))
        goto error;
    fflush(*dest);
    fclose(src2);
    src2 = NULL;
    if (pp_tokenize(*dest, pp_tokens))
        goto error;

    goto cleanup;
error:
    err = 1;
    if (*splice_pos != NULL) {
        free(*splice_pos);
        *splice_pos = NULL;
    }
    if (*dest != NULL) {
        fclose(*dest);
        *dest = NULL;
    }
cleanup:
    if (src != NULL)
        fclose(src);
    if (src1 != NULL)
        fclose(src1);
    if (src2 != NULL)
        fclose(src2);
    return err;
}

enum pp_state {
    PP_STATE__NONE,
    PP_STATE__DIRECTIVE,
    PP_STATE__TEXT,
};

enum pp_directive {
    PP_DIRECTIVE__NONE,
    PP_DIRECTIVE__NON_DIRECTIVE,
    PP_DIRECTIVE__IF,
    PP_DIRECTIVE__IFDEF,
    PP_DIRECTIVE__IFNDEF,
    PP_DIRECTIVE__ELIF,
    PP_DIRECTIVE__ELSE,
    PP_DIRECTIVE__ENDIF,
    PP_DIRECTIVE__INCLUDE,
    PP_DIRECTIVE__DEFINE,
    PP_DIRECTIVE__UNDEF,
    PP_DIRECTIVE__LINE,
    PP_DIRECTIVE__ERROR,
    PP_DIRECTIVE__PRAGMA,
};

enum macro_definition_state {
    MACRO_EXPECT_IDENTIFIER = 0,
    MACRO_EXPECT_LPAREN_OR_WHITESPACE,
    MACRO_EXPECT_REPLACEMENT_LIST_START,
    MACRO_EXPECT_REPLACEMENT_LIST_CONTINUE,
    MACRO_EXPECT_PARAMETERS_START,
    MACRO_EXPECT_PARAMETER,
    MACRO_EXPECT_COMMA,
};

int preprocess_recursive(const char *filename, struct token_list *tokens, struct hashmap *macro_table) {
    int err = 0;
    FILE *src = NULL;
    unsigned long *splice_pos = NULL;
    size_t splice_count;
    struct pp_token_list pp_tokens;
    struct pp_token *pp_token;
    enum pp_state state = PP_STATE__NONE;
    enum pp_directive directive = PP_DIRECTIVE__NONE;
    int directive_state = 0;
    struct macro_entry macro = {0};
    size_t j;
    unsigned long pos;
    struct pp_token **tail = NULL;
    unsigned long line = 1; // TODO
    unsigned int cond_nest_level = 0;
    unsigned int cond_skip_nest_level = 0;
    bool cond_skip = false;
    bool cond_skip_force = false;
    const struct pp_token *directive_pp_token = NULL;
    bool ifdef_defined;
    if (_preprocess(filename, &src, &pp_tokens, &splice_pos, &splice_count))
        goto error;
    for (pp_token = pp_tokens.head; pp_token != NULL; pp_token = pp_token->next) {
        unsigned long i = 0;
        int c;
        bool do_skip;
        enum pp_token_type type = pp_token->type;
        if (type == __NEWLINE) {
            if (state == PP_STATE__DIRECTIVE) {
                switch (directive) {
                case PP_DIRECTIVE__NONE:
                    // null directive
                    break;
                case PP_DIRECTIVE__DEFINE:
                    if (cond_skip)
                        break;
                    // macro definition with non-empty replacement list
                    if (directive_state == MACRO_EXPECT_REPLACEMENT_LIST_CONTINUE) {
                        struct pp_token *iter;
                        unsigned long len = pp_token->pos - pos; // be careful!
                        char *buf;
                        assert(macro.macro.common.replacement_list.head != NULL);
                        buf = malloc(len);
                        if (buf == NULL)
                            goto error;
                        if (fseek(src, pos, SEEK_SET) == EOF ||
                            fread(buf, 1, len, src) != len) {
                            free(buf);
                            goto error;
                        }
                        macro.macro.common.buf = buf;
                        for (iter = macro.macro.common.replacement_list.head; iter != NULL; iter = iter->next)
                            assert(iter->pos + iter->len <= len);
                        break;
                    }
                    // macro definition with no whitespace after identifier
                    if (directive_state == MACRO_EXPECT_LPAREN_OR_WHITESPACE) {
                        macro.type = MACRO_TYPE__OBJECT;
                        directive_state = MACRO_EXPECT_REPLACEMENT_LIST_START;
                        // fallthrough
                    }
                    // macro definition with empty replacement list
                    if (directive_state = MACRO_EXPECT_REPLACEMENT_LIST_START) {
                        assert(macro.macro.common.replacement_list.head == NULL);
                        if (macro.type == MACRO_TYPE__OBJECT) {
                            assert(macro.macro.common.buf == NULL);
                        } else if (macro.type == MACRO_TYPE__FUNCTION) {
                            if (macro.macro.function.params_num != 0) {
                                unsigned long len = pp_token->pos - pos; // be careful!
                                char *buf = malloc(len);
                                if (buf == NULL)
                                    goto error;
                                if (fseek(src, pos, SEEK_SET) == EOF ||
                                    fread(buf, 1, len, src) != len) {
                                    free(buf);
                                    goto error;
                                }
                                macro.macro.common.buf = buf;
                                for (j = 0; j < macro.macro.function.params_num; ++j)
                                    assert(macro.macro.function.params[j].pos +
                                            macro.macro.function.params[j].len <= len);
                            } else
                                assert(macro.macro.common.buf == NULL);
                        } else
                            assert(false);
                        break;
                    }
                    // none of the above
                    goto error;
                case PP_DIRECTIVE__UNDEF:
                    if (cond_skip)
                        break;
                    if (!directive_state)
                        goto error;
                    break;
                case PP_DIRECTIVE__IF:
                    cond_nest_level++;
                    if (!directive_state)
                        goto error;
                    if (cond_skip)
                        break;
                    do_skip = false;
                    // TODO evaluate condition - from directive_pp_token to pp_token
                    if (do_skip) {
                        cond_skip_nest_level = cond_nest_level;
                        cond_skip = true;
                        assert(!cond_skip_force);
                    }
                    break;
                case PP_DIRECTIVE__IFDEF:
                    cond_nest_level++;
                    if (!directive_state)
                        goto error;
                    if (cond_skip)
                        break;
                    do_skip = !ifdef_defined;
                    if (do_skip) {
                        cond_skip_nest_level = cond_nest_level;
                        cond_skip = true;
                        assert(!cond_skip_force);
                    }
                    break;
                case PP_DIRECTIVE__IFNDEF:
                    cond_nest_level++;
                    if (!directive_state)
                        goto error;
                    if (cond_skip)
                        break;
                    do_skip = ifdef_defined;
                    if (do_skip) {
                        cond_skip_nest_level = cond_nest_level;
                        cond_skip = true;
                        assert(!cond_skip_force);
                    }
                    break;
                case PP_DIRECTIVE__ELIF:
                    if (!directive_state)
                        goto error;
                    assert(cond_nest_level > 0);
                    if (!cond_skip) {
                        // we should skip until #endif
                        cond_skip_nest_level = cond_nest_level;
                        cond_skip = true;
                        assert(!cond_skip_force);
                        cond_skip_force = true;
                    } else if (cond_nest_level != cond_skip_nest_level) {
                        // #elif is nested, we should skip
                        assert(cond_nest_level > cond_skip_nest_level);
                    } else if (!cond_skip_force) {
                        do_skip = false;
                        // TODO evaluate condition - from directive_pp_token to pp_token
                        if (!do_skip) {
                            cond_skip = false;
                        }
                    }
                    break;
                case PP_DIRECTIVE__ELSE:
                    assert(!directive_state);
                    assert(cond_nest_level > 0);
                    if (!cond_skip) {
                        // we should skip until #endif
                        cond_skip_nest_level = cond_nest_level;
                        cond_skip = true;
                        assert(!cond_skip_force);
                        cond_skip_force = true;
                    } else if (cond_nest_level != cond_skip_nest_level) {
                        // #else is nested, we should skip
                        assert(cond_nest_level > cond_skip_nest_level);
                    } else if (!cond_skip_force) {
                        // stop skipping
                        cond_skip = false;
                    }
                    break;
                case PP_DIRECTIVE__ENDIF:
                    assert(!directive_state);
                    assert(cond_nest_level > 0);
                    cond_nest_level--;
                    if (cond_skip && cond_nest_level < cond_skip_nest_level) {
                        cond_skip = false;
                        cond_skip_force = false;
                    }
                    break;
                default:
                    // TODO
                    ;
                }
                if (!cond_skip && directive == PP_DIRECTIVE__DEFINE) {
                    struct macro_entry *duplicate;
                    struct macro_entry *entry_ptr;
                    macro_sanitize_replacement_list(&macro);
                    duplicate = macro_table_get(macro_table, macro.macro.name);
                    if (duplicate) {
                        // macro redefinition
                        if (!are_macros_identical(&macro, duplicate))
                            goto error;
                        // free macro
                        macro_entry_free(&macro);
                        memset(&macro, 0, sizeof(macro));
                    } else {
                        entry_ptr = malloc(sizeof(struct macro_entry));
                        if (entry_ptr == NULL)
                            goto error;
                        memcpy(entry_ptr, &macro, sizeof(struct macro_entry));
                        memset(&macro, 0, sizeof(macro));
                        macro_table_add(macro_table, entry_ptr);
                    }
                }
            }
            state = PP_STATE__NONE;
            continue;
        }
        if (state == PP_STATE__NONE) {
            switch (type) {
            case __WHITESPACE:
            case __COMMENT_OLD_STYLE:
            case __COMMENT_NEW_STYLE:
                break;
            case PP_TOKEN__PUNCTUATOR:
                if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                    (c = fgetc(src)) == EOF)
                    goto error;
                if (c == '#') {
                    state = PP_STATE__DIRECTIVE;
                    directive = PP_DIRECTIVE__NONE;
                    break;
                }
                // fallthrough
            default:
                assert(type != PP_TOKEN__HEADER_NAME);
                state = PP_STATE__TEXT;
            }
        } else if (state == PP_STATE__TEXT) {
            assert(type != PP_TOKEN__HEADER_NAME);
            if (cond_skip)
                continue;
        } else if (state == PP_STATE__DIRECTIVE) {
            bool whitespace = false;
            char *name = NULL;
            if (type == __WHITESPACE) {
                if (fseek(src, pp_token->pos, SEEK_SET))
                    goto error;
                for (i = 0; i < pp_token->len; ++i) {
                    switch (fgetc(src)) {
                    case ' ':
                    case '\t':
                        break;
                    default:
                        goto error;
                    }
                }
                whitespace = true;
            } else if (type == __COMMENT_OLD_STYLE || type == __COMMENT_NEW_STYLE) {
                whitespace = true;
            }
            switch (directive) {
            case PP_DIRECTIVE__NONE:
                if (whitespace)
                    break;
                if (type == PP_TOKEN__IDENTIFIER) {
                    char identifier[8] = {0}; // strlen("include") == 7
                    if (pp_token->len > sizeof(identifier) - 1) {
                        directive = PP_DIRECTIVE__NON_DIRECTIVE;
                        continue;
                    }
                    if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                        fread(identifier, 1, pp_token->len, src) != pp_token->len)
                        goto error;
                    if (!strncmp(identifier, "if", sizeof(identifier)))
                        directive = PP_DIRECTIVE__IF;
                    else if (!strncmp(identifier, "ifdef", sizeof(identifier)))
                        directive = PP_DIRECTIVE__IFDEF;
                    else if (!strncmp(identifier, "ifndef", sizeof(identifier)))
                        directive = PP_DIRECTIVE__IFNDEF;
                    else if (!strncmp(identifier, "elif", sizeof(identifier)))
                        directive = PP_DIRECTIVE__ELIF;
                    else if (!strncmp(identifier, "else", sizeof(identifier)))
                        directive = PP_DIRECTIVE__ELSE;
                    else if (!strncmp(identifier, "endif", sizeof(identifier)))
                        directive = PP_DIRECTIVE__ENDIF;
                    else if (!strncmp(identifier, "include", sizeof(identifier)))
                        directive = PP_DIRECTIVE__INCLUDE;
                    else if (!strncmp(identifier, "define", sizeof(identifier)))
                        directive = PP_DIRECTIVE__DEFINE;
                    else if (!strncmp(identifier, "undef", sizeof(identifier)))
                        directive = PP_DIRECTIVE__UNDEF;
                    else if (!strncmp(identifier, "line", sizeof(identifier)))
                        directive = PP_DIRECTIVE__LINE;
                    else if (!strncmp(identifier, "error", sizeof(identifier)))
                        directive = PP_DIRECTIVE__ERROR;
                    else if (!strncmp(identifier, "pragma", sizeof(identifier)))
                        directive = PP_DIRECTIVE__PRAGMA;
                    else
                        directive = PP_DIRECTIVE__NON_DIRECTIVE;
                } else
                    directive = PP_DIRECTIVE__NON_DIRECTIVE;
                directive_state = 0;
                break;
            case PP_DIRECTIVE__ELIF:
                if (!cond_nest_level)
                    goto error;
                // fallthrough
            case PP_DIRECTIVE__IF:
                if (directive_state == 0) {
                    if (whitespace)
                        break;
                    directive_state = 1;
                    directive_pp_token = pp_token;
                }
                break;
            case PP_DIRECTIVE__IFDEF:
            case PP_DIRECTIVE__IFNDEF:
                if (whitespace)
                    break;
                if (directive_state || type != PP_TOKEN__IDENTIFIER)
                    goto error;
                directive_state = 1;
                name = malloc(pp_token->len + 1);
                if (name == NULL)
                    goto error;
                if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                    fread(name, 1, pp_token->len, src) != pp_token->len) {
                    free(name);
                    goto error;
                }
                name[pp_token->len] = '\0';
                ifdef_defined = macro_table_get(macro_table, name) != NULL;
                free(name);
                break;
            case PP_DIRECTIVE__ELSE:
                // NOTE: we don't verify there are no extra #else/#elif before the closing #endif
                // fallthrough
            case PP_DIRECTIVE__ENDIF:
                if (!cond_nest_level)
                    goto error;
                if (whitespace)
                    break;
                goto error;
            case PP_DIRECTIVE__INCLUDE:
                if (cond_skip)
                    break;
                // TODO
                break;
            case PP_DIRECTIVE__DEFINE:
                if (cond_skip)
                    break;
                switch (directive_state) {
                case MACRO_EXPECT_IDENTIFIER:
                    // macro name
                    if (whitespace)
                        break;
                    else if (type != PP_TOKEN__IDENTIFIER)
                        goto error;
                    directive_state = MACRO_EXPECT_LPAREN_OR_WHITESPACE;
                    name = malloc(pp_token->len + 1);
                    if (name == NULL)
                        goto error;
                    if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                        fread(name, 1, pp_token->len, src) != pp_token->len) {
                        free(name);
                        goto error;
                    }
                    name[pp_token->len] = '\0';
                    if (is_macro_name_reserved(name)) {
                        free(name);
                        goto error;
                    }
                    macro_entry_init(&macro, name);
                    break;
                case MACRO_EXPECT_LPAREN_OR_WHITESPACE:
                    // lparen (optional)
                    // there shall be white-space between object-like macro name and replacement list
                    if (whitespace) {
                        macro.type = MACRO_TYPE__OBJECT;
                        directive_state = MACRO_EXPECT_REPLACEMENT_LIST_START;
                    } else {
                        if (pp_token->len != 1)
                            goto error;
                        if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                            (c = fgetc(src)) == EOF)
                            goto error;
                        if (c != '(')
                            goto error;
                        macro.type = MACRO_TYPE__FUNCTION;
                        directive_state = MACRO_EXPECT_PARAMETERS_START;
                    }
                    break;
                case MACRO_EXPECT_REPLACEMENT_LIST_START:
                    // skip leading whitespace before replacement list
                    if (whitespace)
                        break;
                    directive_state = MACRO_EXPECT_REPLACEMENT_LIST_CONTINUE;
                    if (macro.type == MACRO_TYPE__OBJECT)
                        pos = pp_token->pos;
                    tail = &macro.macro.common.replacement_list.head;
                    // fallthrough
                case MACRO_EXPECT_REPLACEMENT_LIST_CONTINUE:
                    // TODO # and ## operators, also __VA_ARGS__
                    *tail = malloc(sizeof(struct pp_token));
                    if (*tail == NULL)
                        goto error;
                    (*tail)->next = NULL;
                    (*tail)->pos = pp_token->pos - pos;
                    (*tail)->len = pp_token->len;
                    (*tail)->type = type;
                    tail = &((*tail)->next);
                    break;
                case MACRO_EXPECT_PARAMETERS_START:
                    // skip leading whitespace before parameters list
                    if (whitespace)
                        break;
                    // quick way to determine number of parameters
                    if (fseek(src, pp_token->pos, SEEK_SET) == EOF)
                        goto error;
                    j = 0;
                    // zero number of parameters is an edge case
                    if ((c = fgetc(src)) == EOF)
                        goto error;
                    if (c != ')') {
                        while ((c = fgetc(src)) != EOF) {
                            if (c == ')' || c == ',')
                                j++;
                            if (c == ')' || c == '.')
                                break;
                        }
                        if (c == EOF)
                            goto error;
                        macro.macro.function.params = malloc(sizeof(*macro.macro.function.params) * j);
                        if (macro.macro.function.params == NULL)
                            goto error;
                    }
                    macro.macro.function.params_num = j;
                    j = 0;
                    directive_state = MACRO_EXPECT_PARAMETER;
                    pos = pp_token->pos;
                    // fallthrough
                case MACRO_EXPECT_PARAMETER:
                    // we ignore whitespace in parameter list
                    if (whitespace)
                        break;
                    if (type == PP_TOKEN__PUNCTUATOR) {
                        unsigned long len = pp_token->len;
                        char buf[4] = {0};
                        if (len > sizeof(buf) - 1)
                            goto error;
                        if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                            fread(buf, 1, len, src) != len)
                            goto error;
                        if (!strncmp(buf, ")", sizeof(buf))) {
                            // rparen
                            assert(j == macro.macro.function.params_num);
                            // otherwise, we expect a comma
                            assert(j == 0 || macro.macro.function.varargs);
                            directive_state = MACRO_EXPECT_REPLACEMENT_LIST_START;
                        } else if (!strncmp(buf, "...", sizeof(buf))) {
                            // ellipsis - should appear only once
                            if (macro.macro.function.varargs)
                                goto error;
                            macro.macro.function.varargs = true;
                        } else
                            goto error;
                        break;
                    }
                    if (macro.macro.function.varargs)
                        goto error;
                    if (type != PP_TOKEN__IDENTIFIER)
                        goto error;
                    if (j >= macro.macro.function.params_num)
                        goto error;
                    macro.macro.function.params[j].pos = pp_token->pos - pos;
                    macro.macro.function.params[j].len = pp_token->len;
                    j++;
                    directive_state = MACRO_EXPECT_COMMA;
                    break;
                case MACRO_EXPECT_COMMA:
                    // we ignore whitespace in parameter list
                    if (whitespace)
                        break;
                    if (type != PP_TOKEN__PUNCTUATOR)
                        goto error;
                    if (pp_token->len != 1)
                        goto error;
                    if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                        (c = fgetc(src)) == EOF)
                        goto error;
                    if (c == ',')
                        directive_state = MACRO_EXPECT_PARAMETER;
                    else if (c == ')') {
                        // rparen
                        assert(j == macro.macro.function.params_num);
                        // otherwise, we expect a parameter
                        assert(j != 0 && !macro.macro.function.varargs);
                        directive_state = MACRO_EXPECT_REPLACEMENT_LIST_START;
                    }
                }
                break;
            case PP_DIRECTIVE__UNDEF:
                if (cond_skip)
                    break;
                if (whitespace)
                    break;
                if (directive_state || type != PP_TOKEN__IDENTIFIER)
                    goto error;
                directive_state = 1;
                name = malloc(pp_token->len + 1);
                if (name == NULL)
                    goto error;
                if (fseek(src, pp_token->pos, SEEK_SET) == EOF ||
                    fread(name, 1, pp_token->len, src) != pp_token->len) {
                    free(name);
                    goto error;
                }
                name[pp_token->len] = '\0';
                if (is_macro_name_reserved(name)) {
                    free(name);
                    goto error;
                }
                macro_table_remove(macro_table, name);
                free(name);
                break;
            case PP_DIRECTIVE__LINE:
                if (cond_skip)
                    break;
                // TODO
                break;
            case PP_DIRECTIVE__ERROR:
                if (cond_skip)
                    break;
                // TODO
                goto error;
            case PP_DIRECTIVE__PRAGMA:
                if (cond_skip)
                    break;
                // TODO
                break;
            case PP_DIRECTIVE__NON_DIRECTIVE:
                if (cond_skip)
                    break;
                // TODO - ignore..
                break;
            default:
                assert(false);
            }
        } else {
            assert(false);
        }
    }

    if (cond_nest_level)
        goto error;

    goto cleanup;
error:
    err = 1;
cleanup:
    macro_entry_free(&macro);
    if (src != NULL)
        fclose(src);
    return err;
}
