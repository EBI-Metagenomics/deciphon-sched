#ifndef HMM_H
#define HMM_H

#include "sched/rc.h"
#include <stdint.h>

enum sched_rc hmm_submit(void *hmm, int64_t job_id);
enum sched_rc hmm_wipe(void);

#endif
