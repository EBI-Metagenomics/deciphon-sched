#include "tok.h"
#include "compiler.h"
#include "dcp_sched/rc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DELIM " \t\r"

static void add_space_before_newline(char *line);
static int next_line(FILE *restrict fd, unsigned size, char *line);

enum tok_id tok_id(struct tok const *tok) { return tok->id; }

char const *tok_value(struct tok const *tok) { return tok->value; }

unsigned tok_size(struct tok const *tok)
{
    return (unsigned)strlen(tok->value);
}

int tok_next(struct tok *tok, FILE *restrict fd)
{
    if (tok->line.consumed)
    {
        int rc = next_line(fd, MEMBER_SIZE(tok->line, data), tok->line.data);
        if (rc && rc == SCHED_NOTFOUND)
        {
            tok->value = NULL;
            tok->id = TOK_EOF;
            tok->line.data[0] = '\0';
            return SCHED_DONE;
        }
        if (rc && rc != SCHED_NOTFOUND) return SCHED_FAIL;
        tok->value = strtok_r(tok->line.data, DELIM, &tok->line.ctx);
        tok->line.number++;
    }
    else
        tok->value = strtok_r(NULL, DELIM, &tok->line.ctx);

    if (!tok->value) return SCHED_FAIL;

    if (!strcmp(tok->value, "\n"))
        tok->id = TOK_NL;
    else
        tok->id = TOK_WORD;

    tok->line.consumed = tok->id == TOK_NL;

    return SCHED_DONE;
}

static int next_line(FILE *restrict fd, unsigned size, char *line)
{
    /* -1 to append space before newline if required */
    assert(size > 0);
    if (!fgets(line, (int)(size - 1), fd))
    {
        if (feof(fd)) return SCHED_NOTFOUND;

        return SCHED_FAIL;
    }

    add_space_before_newline(line);
    return SCHED_DONE;
}

static void add_space_before_newline(char *line)
{
    unsigned n = (unsigned)strlen(line);
    if (n > 0)
    {
        if (line[n - 1] == '\n')
        {
            line[n - 1] = ' ';
            line[n] = '\n';
            line[n + 1] = '\0';
        }
        else
        {
            line[n - 1] = '\n';
            line[n] = '\0';
        }
    }
}
