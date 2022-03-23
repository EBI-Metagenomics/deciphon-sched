#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "sched/limits.h"
#include <stdint.h>

struct sched_db
{
    int64_t id;
    int64_t xxh3_64;
    char filename[FILENAME_SIZE];
    int64_t hmm_id;
};

typedef void(sched_db_set_cb)(struct sched_db *db, void *arg);

void sched_db_init(struct sched_db *db);
enum sched_rc sched_db_get(struct sched_db *db);
enum sched_rc sched_db_add(struct sched_db *db, char const *filename);
enum sched_rc sched_db_get_all(sched_db_set_cb cb, struct sched_db *db,
                               void *arg);

#endif
