#ifndef PROD_H
#define PROD_H

#include "dcp_sched/limits.h"
#include <stdint.h>
#include <stdio.h>

struct prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;
    int64_t match_id;

    char prof_name[DCP_PROF_NAME_SIZE];
    char abc_name[DCP_ABC_NAME_SIZE];

    double loglik;
    double null_loglik;

    char prof_typeid[DCP_PROFILE_TYPEID_SIZE];
    char version[DCP_VERSION_SIZE];

    struct array *match;
};

struct protein_match;

int prod_module_init(void);

int prod_begin_submission(void);
int prod_end_submission(void);

int sched_prod_add(void);
int sched_prod_next(int64_t job_id, int64_t *prod_id);
int sched_prod_get(int64_t prod_id);
void prod_module_del(void);

#endif
