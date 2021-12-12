#ifndef DCP_SCHED_SCHED_H
#define DCP_SCHED_SCHED_H

#include "dcp_sched/export.h"
#include "dcp_sched/job.h"
#include "dcp_sched/limits.h"
#include "dcp_sched/prod.h"
#include "dcp_sched/rc.h"
#include "dcp_sched/version.h"
#include <stdbool.h>
#include <stdint.h>

SCHED_API int sched_setup(char const *filepath);
SCHED_API int sched_open(void);
SCHED_API int sched_close(void);

SCHED_API int sched_add_db(char const *filepath, int64_t *id);

SCHED_API int sched_begin_job_submission(int64_t db_id, bool multi_hits,
                                         bool hmmer3_compat);
SCHED_API void sched_add_seq(char const *name, char const *data);
SCHED_API int sched_end_job_submission(void);

SCHED_API int sched_begin_prod_submission(void);
SCHED_API int sched_end_prod_submission(void);

SCHED_API int sched_next_pending_job(int64_t *job_id);

#endif
