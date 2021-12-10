#ifndef TOK_H
#define TOK_H

#include <stdbool.h>
#include <stdio.h>

enum tok_id
{
    TOK_NL,
    TOK_WORD,
    TOK_EOF,
};

struct tok
{
    unsigned id;
    char const *value;
    struct
    {
        unsigned number;
        bool consumed;
        char *ctx;
        char data[128000];
    } line;
};

#define TOK_DECLARE(var)                                                       \
    struct tok var = {TOK_NL, var.line.data, {0, true, 0, {0}}};

enum tok_id tok_id(struct tok const *tok);
char const *tok_value(struct tok const *tok);
unsigned tok_size(struct tok const *tok);
int tok_next(struct tok *tok, FILE *restrict fd);

#endif
