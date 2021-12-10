#ifndef DCP_SCHED_SCHED_H
#define DCP_SCHED_SCHED_H

#include <stdbool.h>
#include <stdint.h>

int dcp_sched_setup(char const *filepath);
int dcp_sched_open(char const *filepath);
int dcp_sched_close(void);

int dcp_sched_start_job_submission(int64_t db_id, bool multi_hits,
                                   bool hmmer3_compat);
void dcp_sched_add_seq(char const *name, char const *data);
int dcp_sched_end_job_submission(void);

int dcp_sched_start_prod_submission(void);
int dcp_sched_add_prod(void);
int dcp_sched_end_prod_submission(void);

#endif
