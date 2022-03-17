#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "sched/db.h"
#include "sched/job.h"
#include "sched/limits.h"
#include "sched/logger.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/seq.h"

struct sqlite3;

enum sched_rc sched_setup(char const *filepath);
enum sched_rc sched_open(void);
enum sched_rc sched_close(void);
enum sched_rc sched_wipe(void);
struct sqlite3 *sched_handle(void);

#endif
