#ifndef DCP_SCHED_PROD_H
#define DCP_SCHED_PROD_H

#include <stdio.h>

typedef int(dcp_sched_prod_write_match_cb)(FILE *fp, void const *match);

void dcp_sched_prod_set_job_id(int64_t);
void dcp_sched_prod_set_seq_id(int64_t);
void dcp_sched_prod_set_match_id(int64_t);

void dcp_sched_prod_set_profile_name(char const *);
void dcp_sched_prod_set_abc_name(char const *);

void dcp_sched_prod_set_alt_loglik(double);
void dcp_sched_prod_set_null_loglik(double);

void dcp_sched_prod_set_profile_typeid(char const *);
void dcp_sched_prod_set_version(char const *);

int dcp_sched_prod_write_preamble(void);
int dcp_sched_prod_write_match(dcp_sched_prod_write_match_cb *cb,
                               void const *match);
int dcp_sched_prod_write_match_sep(void);
int dcp_sched_prod_write_nl(void);

#endif
