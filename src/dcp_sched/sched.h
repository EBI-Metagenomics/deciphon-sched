#ifndef DCP_SCHED_SCHED_H
#define DCP_SCHED_SCHED_H

#include "dcp_sched/job.h"
#include "dcp_sched/limits.h"
#include "dcp_sched/prod.h"
#include "dcp_sched/rc.h"
#include "dcp_sched/version.h"
#include <stdbool.h>
#include <stdint.h>

int sched_setup(char const *filepath);
int sched_open(void);
int sched_close(void);

int sched_add_db(char const *filepath, int64_t *id);

int sched_begin_job_submission(int64_t db_id, bool multi_hits,
                               bool hmmer3_compat);
void sched_add_seq(char const *name, char const *data);
int sched_end_job_submission(void);

int sched_begin_prod_submission(void);
int sched_end_prod_submission(void);

int sched_next_pending_job(int64_t *job_id);

#endif
