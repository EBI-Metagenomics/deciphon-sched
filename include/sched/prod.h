#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "sched/structs.h"
#include <stdint.h>
#include <stdio.h>

void sched_prod_init(struct sched_prod *, int64_t scan_id);
enum sched_rc sched_prod_get_by_id(struct sched_prod *, int64_t id);
enum sched_rc sched_prod_add(struct sched_prod *);
enum sched_rc sched_prod_add_file(char const *filename);

enum sched_rc sched_prod_get_all(void (*callb)(struct sched_prod *,
                                               struct sched_hmmer *, void *),
                                 struct sched_prod *, struct sched_hmmer *,
                                 void *arg);

#endif
