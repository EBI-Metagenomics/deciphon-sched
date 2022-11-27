#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "sched/db.h"
#include "sched/error.h"
#include "sched/hmm.h"
#include "sched/hmmer.h"
#include "sched/job.h"
#include "sched/limits.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/scan.h"
#include "sched/seq.h"

struct sched_health
{
    FILE *fp;
    int num_errors;
};

enum sched_rc sched_init(char const *filepath);
enum sched_rc sched_cleanup(void);
enum sched_rc sched_health_check(struct sched_health *);
enum sched_rc sched_wipe(void);

#endif
