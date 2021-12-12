#ifndef SEQ_QUEUE_H
#define SEQ_QUEUE_H

#include <stdint.h>

void seq_queue_init(void);
void seq_queue_add(int64_t job_id, char const *name, char const *data);
unsigned seq_queue_size(void);
struct sched_seq *seq_queue_get(unsigned i);

#endif
