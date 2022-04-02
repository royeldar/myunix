#ifndef __TRANSLATE_H__
#define __TRANSLATE_H__

#include <stdio.h>

#include "token.h"

int sanitize_source(FILE *src, FILE *dest);
int replace_trigraphs(FILE *src, FILE *dest);
int splice_lines(FILE *src, FILE *dest, unsigned long **pos, size_t *n);
int pp_tokenize(FILE *src, struct pp_token_list *pp_tokens);

#endif // __TRANSLATE_H__
