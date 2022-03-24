#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "sched/db.h"
#include "sched/hmm.h"
#include "sched/job.h"
#include "sched/limits.h"
#include "sched/logger.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/scan.h"
#include "sched/seq.h"

struct sqlite3;

enum sched_rc sched_init(char const *filepath);
enum sched_rc sched_cleanup(void);
enum sched_rc sched_wipe(void);

#endif
