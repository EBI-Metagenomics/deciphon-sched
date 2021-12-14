#ifndef DCP_SCHED_PROD_H
#define DCP_SCHED_PROD_H

#include "dcp_sched/export.h"
#include "dcp_sched/limits.h"
#include <stdint.h>
#include <stdio.h>

struct sched_prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;

    char profile_name[SCHED_PROFILE_NAME_SIZE];
    char abc_name[SCHED_ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[SCHED_PROFILE_TYPEID_SIZE];
    char version[SCHED_VERSION_SIZE];

    char match[SCHED_MATCH_SIZE];
};

typedef int(sched_prod_write_match_cb)(FILE *fp, void const *match);

SCHED_API void sched_prod_init(struct sched_prod *prod, int64_t job_id);
SCHED_API int sched_prod_next(struct sched_prod *prod);

SCHED_API int sched_prod_write_begin(struct sched_prod const *prod,
                                     unsigned thread_num);
SCHED_API int sched_prod_write_match(sched_prod_write_match_cb *cb,
                                     void const *match, unsigned thread_num);
SCHED_API int sched_prod_write_match_sep(unsigned thread_num);
SCHED_API int sched_prod_write_end(unsigned thread_num);

#endif
