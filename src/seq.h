#ifndef SEQ_H
#define SEQ_H

#include "dcp_sched/limits.h"
#include <stdint.h>

struct seq
{
    int64_t id;
    int64_t job_id;
    char name[SCHED_SEQ_NAME_SIZE];
    char data[SCHED_SEQ_SIZE];
};

int seq_module_init(void);
void seq_init(struct seq *s, int64_t job_id, char const *name,
              char const *data);
int seq_submit(struct seq *seq);
int seq_next(int64_t job_id);
int seq_get(int64_t id);
void seq_module_del(void);

#endif
