#ifndef DCP_SCHED_PROD_H
#define DCP_SCHED_PROD_H

#include <stdint.h>
#include <stdio.h>

typedef int(sched_prod_write_match_cb)(FILE *fp, void const *match);

void sched_prod_set_job_id(int64_t);
void sched_prod_set_seq_id(int64_t);
void sched_prod_set_match_id(int64_t);

void sched_prod_set_profile_name(char const *);
void sched_prod_set_abc_name(char const *);

void sched_prod_set_alt_loglik(double);
void sched_prod_set_null_loglik(double);

void sched_prod_set_profile_typeid(char const *);
void sched_prod_set_version(char const *);

int sched_prod_write_begin(void);
int sched_prod_write_match(sched_prod_write_match_cb *cb, void const *match);
int sched_prod_write_match_sep(void);
int sched_prod_write_end(void);

#endif
