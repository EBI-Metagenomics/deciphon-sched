#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "sched/limits.h"
#include <stdint.h>

struct sched_db
{
    int64_t id;
    int64_t xxh3;
    char filename[FILENAME_SIZE];
    int64_t hmm_id;
};

typedef void(sched_db_set_func_t)(struct sched_db *, void *arg);

void sched_db_init(struct sched_db *);

enum sched_rc sched_db_get_by_id(struct sched_db *, int64_t id);
enum sched_rc sched_db_get_by_xxh3(struct sched_db *, int64_t xxh3);
enum sched_rc sched_db_get_by_filename(struct sched_db *, char const *filename);

enum sched_rc sched_db_get_all(sched_db_set_func_t, struct sched_db *,
                               void *arg);

enum sched_rc sched_db_add(struct sched_db *, char const *filename);

enum sched_rc sched_db_remove(int64_t id);

#endif
