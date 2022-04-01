#ifndef PROD_H
#define PROD_H

struct sched_prod;

enum sched_rc prod_begin_submission(unsigned nfiles);
enum sched_rc prod_end_submission(void);
enum sched_rc prod_get(struct sched_prod *prod);
enum sched_rc prod_scan_next(struct sched_prod *prod);
enum sched_rc prod_next(struct sched_prod *prod);
enum sched_rc prod_delete(void);

#endif
