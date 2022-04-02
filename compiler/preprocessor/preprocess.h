#ifndef __PREPROCESS_H__
#define __PREPROCESS_H__

#include <stdio.h>
#include <stdbool.h>

#include "token.h"
#include "../common/hashmap.h"

int preprocess(const char *filename, struct token_list *tokens);

// TODO move to macros.h or something

#endif // __PREPROCESS_H__
