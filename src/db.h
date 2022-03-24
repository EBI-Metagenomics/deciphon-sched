#ifndef DB_H
#define DB_H

#include "sched/db.h"
#include "sched/limits.h"
#include <stdint.h>

enum sched_rc db_has(char const *filename, struct sched_db *);
enum sched_rc db_hash(char const *filename, int64_t *xxh64);
enum sched_rc db_get_by_id(struct sched_db *, int64_t id);
enum sched_rc db_get_by_xxh3(struct sched_db *, int64_t xxh3);
enum sched_rc db_delete(void);

#endif
