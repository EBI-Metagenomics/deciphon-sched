#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "dcp_sched/limits.h"
#include <stdint.h>

struct db
{
    int64_t id;
    int64_t xxh64;
    char filepath[DCP_PATH_SIZE];
};

int db_module_init(void);
int db_add(char const *filepath, int64_t *id);
int db_has(char const *filepath);
int db_hash(char const *filepath, int64_t *xxh64);
int db_get_by_id(struct db *db, int64_t id);
int db_get_by_xxh64(struct db *db, int64_t xxh64);
void db_module_del(void);

#endif
