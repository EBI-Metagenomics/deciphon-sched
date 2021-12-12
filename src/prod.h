#ifndef PROD_H
#define PROD_H

#include "dcp_sched/limits.h"
#include "dcp_sched/sched.h"
#include <stdint.h>
#include <stdio.h>

struct protein_match;

int prod_module_init(void);

int prod_begin_submission(void);
int prod_end_submission(void);

int sched_prod_add(void);
int sched_prod_next(int64_t job_id, int64_t *prod_id);
int sched_prod_get(int64_t prod_id);
void prod_module_del(void);

#endif
