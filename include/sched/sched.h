#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "limits.h"
#include "sched/db.h"
#include "sched/job.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/seq.h"

enum sched_rc sched_setup(char const *filepath);
enum sched_rc sched_open(void);
enum sched_rc sched_close(void);

#endif
