#ifndef __TOKEN_H__
#define __TOKEN_H__

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
    unsigned long pos, len;
    enum pp_token_type type;
};

struct pp_token_list {
    struct pp_token *head;
};

#endif // __TOKEN_H__
