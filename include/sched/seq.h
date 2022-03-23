#ifndef SCHED_SEQ_H
#define SCHED_SEQ_H

#include "sched/limits.h"
#include <stdint.h>

struct sched_seq
{
    int64_t id;
    int64_t scan_id;
    char name[SEQ_NAME_SIZE];
    char data[SEQ_SIZE];
};

void sched_seq_init(struct sched_seq *seq, int64_t scan_id, char const *name,
                    char const *data);

enum sched_rc sched_seq_get_by_id(struct sched_seq *, int64_t id);
enum sched_rc sched_seq_next(struct sched_seq *);

#endif
