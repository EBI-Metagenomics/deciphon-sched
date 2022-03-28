#ifndef SCHED_HEALTH_H
#define SCHED_HEALTH_H

#include <stdio.h>

struct sched_db;
struct sched_hmm;

void health_check_db(struct sched_db *db, void *arg);
void health_check_hmm(struct sched_hmm *hmm, void *arg);

#endif
