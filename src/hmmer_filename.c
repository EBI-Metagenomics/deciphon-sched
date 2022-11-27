#include "sched/hmmer_filename.h"
#include "ltoa.h"
#include "sched/limits.h"
#include "sched/rc.h"
#include "strlcpy.h"
#include "to.h"
#include <string.h>

void sched_hmmer_filename_setup(struct sched_hmmer_filename const *x,
                                char *filename)
{
    char *p = filename;

    p += strlen(strcpy(p, "hmmer"));

    p += strlen(strcpy(p, "_"));
    p += sched_ltoa(p, x->scan_id);

    p += strlen(strcpy(p, "_"));
    p += sched_ltoa(p, x->seq_id);

    p += strlen(strcpy(p, "_"));
    p += strlen(strcpy(p, x->profile_name));

    p += strlen(strcat(p, ".h3r"));
    *p = '\0';
}

enum sched_rc sched_hmmer_filename_parse(struct sched_hmmer_filename *x,
                                         char const *filename)
{
    char name[SCHED_FILENAME_SIZE] = {0};
    size_t n = SCHED_FILENAME_SIZE;
    if (sched_strlcpy(name, filename, n) >= n) return SCHED_FAIL_PARSE_FILENAME;

    char *ctx = NULL;
    char *tok = strtok_r(name, "_", &ctx);
    if (!tok) return SCHED_FAIL_PARSE_FILENAME;
    if (strcmp(tok, "hmmer")) return SCHED_FAIL_PARSE_FILENAME;

    if (!(tok = strtok_r(NULL, "_", &ctx))) return SCHED_FAIL_PARSE_FILENAME;
    if (!to_int64(tok, &x->scan_id)) return SCHED_FAIL_PARSE_FILENAME;

    if (!(tok = strtok_r(NULL, "_", &ctx))) return SCHED_FAIL_PARSE_FILENAME;
    if (!to_int64(tok, &x->seq_id)) return SCHED_FAIL_PARSE_FILENAME;

    if (!(tok = strtok_r(NULL, ".", &ctx))) return SCHED_FAIL_PARSE_FILENAME;
    if (!strcpy(x->profile_name, tok)) return SCHED_FAIL_PARSE_FILENAME;

    if (!(tok = strtok_r(NULL, "", &ctx))) return SCHED_FAIL_PARSE_FILENAME;
    if (strcmp(tok, "h3r")) return SCHED_FAIL_PARSE_FILENAME;

    return 0;
}
