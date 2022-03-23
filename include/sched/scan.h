#ifndef SCHED_SCAN_H
#define SCHED_SCAN_H

#include "sched/limits.h"
#include <stdbool.h>
#include <stdint.h>

struct sched_scan
{
    int64_t id;
    int64_t db_id;

    int multi_hits;
    int hmmer3_compat;

    int64_t job_id;
};

struct sched_prod;
struct sched_seq;

typedef void(sched_seq_set_func_t)(struct sched_seq *seq, void *arg);
typedef void(sched_prod_set_func_t)(struct sched_prod *prod, void *arg);

void sched_scan_init(struct sched_scan *, int64_t db_id, bool multi_hits,
                     bool hmmer3_compat);

enum sched_rc sched_scan_get_seqs(int64_t job_id, sched_seq_set_func_t,
                                  struct sched_seq *seq, void *arg);

enum sched_rc sched_scan_get_prods(int64_t job_id, sched_prod_set_func_t,
                                   struct sched_prod *prod, void *arg);

enum sched_rc sched_scan_get_by_scan_id(struct sched_scan *, int64_t scan_id);
enum sched_rc sched_scan_get_by_job_id(struct sched_scan *, int64_t job_id);

void sched_scan_add_seq(struct sched_scan *, char const *name,
                        char const *data);

#endif
