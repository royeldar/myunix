#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../common/environment.h"
#include "../common/ucn.h"
#include "translate.h"

// convert crlf to lf, make sure there is newline at end of file
int sanitize_source(FILE *src, FILE *dest) {
    int c;
    bool newline = false;
    bool empty = true;
    rewind(src);
    while ((c = fgetc(src)) != EOF) {
        if (c == '\0')
            goto error;
        if (c == '\r')
            // ignore carriage return
            continue;
        else if (fputc(c, dest) == EOF)
            goto error;
        newline = c == '\n';
        empty = false;
    }
    if (ferror(src))
        goto error;
    // make sure there is newline at the end, or the file is empty
    if (!empty && !newline && fputc('\n', dest) == EOF)
        goto error;

    return 0;
error:
    return 1;
}

static inline bool trigraph_convert(int *c) {
    switch (*c) {
    case '=':
        *c = '#'; break;
    case '(':
        *c = '['; break;
    case '/':
        *c = '\\'; break;
    case ')':
        *c = ']'; break;
    case '\'':
        *c = '^'; break;
    case '<':
        *c = '{'; break;
    case '!':
        *c = '|'; break;
    case '>':
        *c = '}'; break;
    case '-':
        *c = '~'; break;
    default:
        return false;
    }
    return true;
}

// replace trigraph sequences
int replace_trigraphs(FILE *src, FILE *dest) {
    int c;
    short i = 0;
    rewind(src);
    while ((c = fgetc(src)) != EOF) {
        if (c == '?') {
            if (i == 2) {
                // ???
                if (fputc('?', dest) == EOF)
                    goto error;
            } else
                i++;
        } else {
            if (i == 2) {
                // ??
                if (trigraph_convert(&c))
                    i = 0;
            }
            while (i--)
                if (fputc('?', dest) == EOF)
                    goto error;
            i = 0;
            if (fputc(c, dest) == EOF)
                goto error;
        }
    }
    if (ferror(src))
        goto error;
    while (i--)
        if (fputc('?', dest) == EOF)
            goto error;

    return 0;
error:
    return 1;
}

// do line splicing
int splice_lines(FILE *src, FILE *dest, unsigned long **pos, size_t *n) {
    static char line[___LOGICAL_SOURCE_LINE_MAXLEN + 1];
    long j;
    size_t count = 0;
    int c, c1 = EOF;
    // quick way to determine number of line splices
    rewind(src);
    while ((c = fgetc(src)) != EOF) {
        if (c == '\n' && c1 == '\\')
            count++;
        c1 = c;
    }
    if (ferror(src))
        goto error;
    *n = count;
    if (count != 0) {
        *pos = malloc(count * sizeof(unsigned long));
        if (*pos == NULL)
            goto error;
    }
    count = 0;
    rewind(src);
    while (!feof(src)) {
        size_t i;
        bool is_splicing = false;
        line[0] = '\0';
        for (i = 0; i < sizeof(line);) {
            char *s = fgets(line + i, sizeof(line) - i, src);
            if (ferror(src))
                goto error;
            if (s == NULL && is_splicing)
                // backslash at last line
                goto error;
            if (s == NULL || *s == '\n')
                // EOF or empty line encountered
                break;
            // can't be empty string
            i += strlen(s) - 1;
            if (line[i] != '\n')
                // logical line exceeding max length
                goto error;
            if (line[i - 1] != '\\')
                // stop splicing lines
                break;
            // delete backslash
            if ((j = ftell(src)) == -1)
                goto error;
            assert(count < *n);
            assert(j >= 2 * (count + 1));
            j -= 2 * (count + 1);
            (*pos)[count++] = j;
            i -= 1;
            is_splicing = true;
        }
        if (fputs(line, dest) == EOF)
            goto error;
    }
    assert(count == *n);

    return 0;
error:
    if (*pos != NULL) {
        free(*pos);
        *pos = NULL;
    }
    return 1;
}

static inline unsigned int hex_val(int c) {
    if (isdigit(c))
        return c - '0';
    switch (tolower(c)) {
    case 'a':
        return 10;
    case 'b':
        return 11;
    case 'c':
        return 12;
    case 'd':
        return 13;
    case 'e':
        return 14;
    case 'f':
        return 15;
    default:
        assert(false);
    }
}

// creates linked list of preprocessing tokens
int pp_tokenize(FILE *src, struct pp_token_list *pp_tokens) {
    struct pp_token **tail = &pp_tokens->head;
    struct pp_token *head;
    long pos = 0, pos1;
    long i = 0, j;
    enum pp_token_type type;
    bool term = true;
    long seek;
    int c, c1;
    int include_directive = 1;
    char include_term;
    int escape_sequence;
    int ucn = 0;
    unsigned long ucn_val;
    rewind(src);
    *tail = NULL;
    while ((c = fgetc(src)) != EOF) {
        if (i == 0) {
            term = false;
            seek = 0;
            type = 0;
            c1 = EOF;
            if (c == '\n') {
                type = __NEWLINE;
                term = true;
            } else if (isspace(c))
                type = __WHITESPACE;
            else if (isdigit(c))
                type = PP_TOKEN__PP_NUMBER;
            else if (isalpha(c) || c == '_')
                type = PP_TOKEN__IDENTIFIER;
            else switch (c) {
            case '[':
            case ']':
            case '(':
            case ')':
            case '{':
            case '}':
            case '~':
            case '?':
            case ';':
            case ',':
                term = true;
                // fallthrough
            case '-':
            case '+':
            case '&':
            case '*':
            case '!':
            case '%':
            case '>':
            case ':':
            case '=':
            case '^':
            case '|':
            case '#':
            case '.': // may be number
            case '/': // may be comment
                type = PP_TOKEN__PUNCTUATOR;
                break;
            case '<': // may be header name
                if (include_directive == 3) {
                    type = PP_TOKEN__HEADER_NAME;
                    include_term = '>';
                } else
                    type = PP_TOKEN__PUNCTUATOR;
                break;
            case '\'':
                type = PP_TOKEN__CHARACTER_CONSTANT;
                escape_sequence = 0;
                break;
            case '"': // may be header name
                if (include_directive == 3) {
                    type = PP_TOKEN__HEADER_NAME;
                    include_term = '"';
                } else {
                    type = PP_TOKEN__STRING_LITERAL;
                    escape_sequence = 0;
                }
                break;
            case '\\': // may be universal character name
                ucn = 1;
                ucn_val = 0;
                break;
            default:
                type = PP_TOKEN__OTHER;
                term = true;
            }
        } else if (ucn == 1) {
            if (c == 'u')
                ucn = 2;
            else if (c == 'U')
                ucn = 6;
            else { // '\'
                ucn = 0;
                term = true;
                if (type)
                    seek = -2;
                else {
                    assert(i == 1);
                    type = PP_TOKEN__OTHER;
                    seek = -1;
                }
            }
        } else if (ucn != 0) {
            if (isxdigit(c)) {
                ucn_val *= 16;
                ucn_val += hex_val(c);
                if (ucn == 5 || ucn == 13) {
                    // universal character name
                    ucn = 0;
                    if (!is_ucn_valid(ucn_val))
                        goto error;
                    switch (type) {
                    case PP_TOKEN__CHARACTER_CONSTANT:
                    case PP_TOKEN__STRING_LITERAL:
                        break;
                    case PP_TOKEN__PP_NUMBER:
                    case PP_TOKEN__IDENTIFIER:
                        if (!is_ucn_valid_identifier(ucn_val))
                            goto error;
                        break;
                    case 0:
                        // initial ucn in identifier
                        if (!is_ucn_valid_identifier_initial(ucn_val))
                            goto error;
                        type = PP_TOKEN__IDENTIFIER;
                        break;
                    default:
                        assert(false);
                    }
                } else
                    ucn++;
            } else { // '\'
                if (type == PP_TOKEN__CHARACTER_CONSTANT || type == PP_TOKEN__STRING_LITERAL)
                    goto error;
                term = true;
                if (ucn >= 6)
                    ucn -= 4;
                if (type)
                    seek = -ucn - 1;
                else {
                    assert(i == ucn);
                    type = PP_TOKEN__OTHER;
                    seek = -ucn;
                }
                ucn = 0;
            }
        } else if (type == __WHITESPACE) {
            if (c == '\n' || !isspace(c)) {
                term = true;
                seek = -1;
            }
        } else if (type == __COMMENT_NEW_STYLE) {
            if (c == '\n') {
                term = true;
                seek = -1;
            }
        } else if (type == __COMMENT_OLD_STYLE) {
            if (c == '/' && c1 == '*') // */
                term = true;
        } else if (type == PP_TOKEN__PP_NUMBER) {
            if (c == '\\') { // may be universal character name
                ucn = 1;
                ucn_val = 0;
            } else if (!(isalnum(c) || c == '.' || c == '_' ||
                ((c == '+' || c == '-') && (c1 == 'e' || c1 == 'E' || c1 == 'p' || c1 == 'P')))) {
                term = true;
                seek = -1;
            }
        } else if (type == PP_TOKEN__IDENTIFIER) {
            bool w = (i == 1 && c1 == 'L');
            if (w && c == '\'') {
                type = PP_TOKEN__CHARACTER_CONSTANT;
                escape_sequence = 0;
            } else if (w && c == '"') {
                type = PP_TOKEN__STRING_LITERAL;
                escape_sequence = 0;
            } else if (c == '\\') { // may be universal character name
                ucn = 1;
                ucn_val = 0;
            } else if (!(isalnum(c) || c == '_')) {
                term = true;
                seek = -1;
            }
        } else if (type == PP_TOKEN__HEADER_NAME) {
            if (include_directive == 3) {
                j = 0;
                if (c == include_term)
                    include_directive = 4;
                else if (c == '\n')
                    include_directive = 0;
            } else if (include_directive == 4) {
                if (c == '\n')
                    include_directive = 5;
                else if (isspace(c))
                    j++;
                else
                    include_directive = 0;
            } else
                assert(false);
            if (include_directive == 5) {
                term = true;
                seek = -j - 1;
            } else if (include_directive == 0) {
                if (fseek(src, pos, SEEK_SET))
                    goto error;
                i = 0;
                continue;
            }
        } else if (type == PP_TOKEN__CHARACTER_CONSTANT || type == PP_TOKEN__STRING_LITERAL) {
            char quote = type == PP_TOKEN__CHARACTER_CONSTANT ? '\'' : '"';
            if (c == '\n')
                goto error;
            switch (escape_sequence) {
            case 0:
            case 4:
                if (c == quote)
                    term = true;
                else if (c == '\\')
                    escape_sequence = 1;
                break;
            case 1:
                if (isdigit(c) && c != '8' && c != '9')
                    escape_sequence = 2;
                else switch (c) {
                case '\'':
                case '"':
                case '?':
                case '\\':
                case 'a':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'v':
                    escape_sequence = 0;
                    break;
                case 'x':
                    escape_sequence = 5;
                    break;
                case 'u': // universal character name
                    escape_sequence = 0;
                    ucn = 2;
                    ucn_val = 0;
                    break;
                case 'U': // universal character name
                    escape_sequence = 0;
                    ucn = 6;
                    ucn_val = 0;
                    break;
                default:
                    goto error;
                }
                break;
            case 2:
            case 3:
                if (isdigit(c) && c != '8' && c != '9')
                    escape_sequence++;
                else if (c == quote)
                    term = true;
                else
                    escape_sequence = c == '\\';
                break;
            case 5:
                if (!isxdigit(c))
                    goto error;
                escape_sequence = 6;
                break;
            case 6:
                if (!isxdigit(c))
                    escape_sequence = 0;
                if (c == quote)
                    term = true;
                break;
            default:
                assert(false);
            }
        } else if (type == PP_TOKEN__PUNCTUATOR) {
            term = true;
            switch (i) {
            case 1:
                switch (c1) {
                case '-':
                    if (c == '>') // ->
                        break;
                    // fallthrough
                case '+':
                case '&':
                case '|':
                    if (c == c1) // --, ++, &&, ||
                        break;
                    // fallthrough
                case '*':
                case '!':
                case '=':
                case '^':
                    if (c == '=') // -=, +=, &=, |=, *=, !=, ==
                        break;
                    seek = -1;
                    break;
                case '%':
                    if (c == '>' || c == '=') // %>, %=
                        break;
                    if (c == ':') { // %:
                        term = false;
                        break;
                    }
                    seek = -1;
                    break;
                case '<':
                    if (c == '%' || c == ':' || c == '=') // <%, <:, <=
                        break;
                    if (c == '<') { // <<
                        term = false;
                        break;
                    }
                    seek = -1;
                    break;
                case '>':
                    if (c == '=') // >=
                        break;
                    if (c == '>') { // >>
                        term = false;
                        break;
                    }
                    seek = -1;
                    break;
                case ':':
                    if (c != '>') // else :>
                        seek = -1;
                    break;
                case '#':
                    if (c != '#') // else ##
                        seek = -1;
                    break;
                case '.':
                    term = false;
                    if (isdigit(c))
                        type = PP_TOKEN__PP_NUMBER;
                    else if (c != '.') { // .
                        term = true;
                        seek = -1;
                    }
                    break;
                case '/':
                    if (c == '=') // /=
                        break;
                    term = false;
                    if (c == '/') // //
                        type = __COMMENT_NEW_STYLE;
                    else if (c == '*') // /*
                        type = __COMMENT_OLD_STYLE;
                    else { // /
                        term = true;
                        seek = -1;
                    }
                    break;
                default:
                    assert(false);
                }
                break;
            case 2:
                switch (c1) {
                case ':': // %:
                    if (c == '%') // %:%
                        term = false;
                    else
                        seek = -1;
                    break;
                case '<': // <<
                case '>': // >>
                    if (c != '=') // else <<=, >>=
                        seek = -1;
                    break;
                case '.': // ..
                    if (c != '.') // else ...
                        seek = -2;
                    break;
                default:
                    assert(false);
                }
                break;
            case 3:
                assert(c1 == '%'); // %:%
                if (c != ':') // else %:%:
                    seek = -2;
                break;
            default:
                assert(false);
            }
        }
        assert(!seek || term);
        i += seek;
        if (seek && fseek(src, seek, SEEK_CUR))
            goto error;
        if (term) {
            char include[7];
            assert(!ucn);
            switch (type) {
            case __NEWLINE:
                include_directive = 1;
                break;
            case __WHITESPACE:
                break;
            case PP_TOKEN__PUNCTUATOR:
                if (include_directive == 1 && i == 0 && c1 == '#')
                    include_directive = 2;
                else
                    include_directive = 0;
                break;
            case PP_TOKEN__IDENTIFIER:
                if (include_directive == 2 && i == 6) { // strlen("include") == 7
                    if (fseek(src, -7, SEEK_CUR))
                        goto error;
                    if (fread(include, 1, 7, src) != 7)
                        goto error;
                    if (!memcmp(include, "include", 7)) {
                        include_directive = 3;
                        break;
                    }
                } else
                    include_directive = 0;
                break;
            default:
                include_directive = 0;
            }
            *tail = malloc(sizeof(struct pp_token));
            if (!*tail)
                goto error;
            (*tail)->next = NULL;
            (*tail)->pos = pos;
            (*tail)->len = i + 1;
            (*tail)->type = type;
            tail = &((*tail)->next);
            pos1 = pos + i + 1;
            i = 0;
            if ((pos = ftell(src)) == -1)
                goto error;
            assert(pos == pos1);
            continue;
        }
        i++;
        c1 = c;
    }
    if (ferror(src))
        goto error;

    if (!term) {
        assert(type == __COMMENT_OLD_STYLE);
        goto error;
    }

    assert(*tail == NULL || type == __NEWLINE);

    return 0;
error:
    head = pp_tokens->head;
    while (head != NULL) {
        struct pp_token *next = head->next;
        free(head);
        head = next;
    }
    return 1;
}
