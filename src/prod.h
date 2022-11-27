#ifndef PROD_H
#define PROD_H

#include <stdio.h>

struct sched_prod;

typedef int prod_add_cb(struct sched_prod const *, void *);

enum sched_rc sched_prod_add_transaction(FILE *fp, prod_add_cb *, void *arg);
enum sched_rc prod_scan_next(struct sched_prod *prod);
enum sched_rc prod_next(struct sched_prod *prod);
enum sched_rc prod_wipe(void);

#endif
