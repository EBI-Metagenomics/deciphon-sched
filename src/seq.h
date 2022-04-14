#ifndef SEQ_H
#define SEQ_H

#include <stdint.h>

struct sched_seq;

enum sched_rc seq_submit(struct sched_seq *seq);
enum sched_rc seq_wipe(void);

#endif
