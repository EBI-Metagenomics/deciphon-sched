#ifndef SCHED_SEQ_H
#define SCHED_SEQ_H

#include "sched/structs.h"
#include <stdint.h>

typedef void(sched_seq_set_func_t)(struct sched_seq *, void *arg);

void sched_seq_init(struct sched_seq *seq, int64_t seq_id, int64_t scan_id,
                    char const *name, char const *data);

enum sched_rc sched_seq_get_by_id(struct sched_seq *, int64_t id);
enum sched_rc sched_seq_scan_next(struct sched_seq *);
enum sched_rc sched_seq_get_all(sched_seq_set_func_t fn, struct sched_seq *,
                                void *arg);

#endif
