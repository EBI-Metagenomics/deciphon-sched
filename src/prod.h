#ifndef PROD_H
#define PROD_H

#include "sched/limits.h"
#include "sched/prod.h"
#include "sched/sched.h"
#include <stdint.h>
#include <stdio.h>

struct protein_match;

enum sched_rc prod_begin_submission(unsigned nfiles);
enum sched_rc prod_end_submission(void);
enum sched_rc prod_get(struct sched_prod *prod);
enum sched_rc prod_next(struct sched_prod *prod);
enum sched_rc prod_delete(void);

#endif
