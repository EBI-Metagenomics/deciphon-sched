#ifndef SEQ_H
#define SEQ_H

#include "sched/seq.h"
#include <stdint.h>

enum sched_rc seq_submit(struct sched_seq *seq);
enum sched_rc seq_get(struct sched_seq *seq);

#endif
