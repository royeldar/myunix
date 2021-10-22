#ifndef __TRANSLATE_H__
#define __TRANSLATE_H__

#include <stdio.h>

enum pp_token_type {
    PP_TOKEN__HEADER_NAME = 1,
    PP_TOKEN__IDENTIFIER,
    PP_TOKEN__PP_NUMBER,
    PP_TOKEN__CHARACTER_CONSTANT,
    PP_TOKEN__STRING_LITERAL,
    PP_TOKEN__PUNCTUATOR,
    PP_TOKEN__OTHER,
    __COMMENT_OLD_STYLE,
    __COMMENT_NEW_STYLE,
    __WHITESPACE,
    __NEWLINE,
};

struct pp_token {
    struct pp_token *next;
    fpos_t pos;
    size_t len;
    enum pp_token_type type;
};

struct pp_token_list {
    struct pp_token *head;
};

int sanitize_source(FILE *src, FILE *dest);
int replace_trigraphs(FILE *src, FILE *dest);
int splice_lines(FILE *src, FILE *dest);
int pp_tokenize(FILE *src, struct pp_token_list *pp_tokens);

#endif // __TRANSLATE_H__
