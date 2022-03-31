#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "sched/limits.h"
#include <stdint.h>
#include <stdio.h>

struct sched_prod
{
    int64_t id;

    int64_t scan_id;
    int64_t seq_id;

    char profile_name[PROFILE_NAME_SIZE];
    char abc_name[ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[PROFILE_TYPEID_SIZE];
    char version[VERSION_SIZE];

    char match[MATCH_SIZE];
};

typedef int(sched_prod_write_match_func_t)(FILE *fp, void const *match);
typedef void(sched_prod_set_func_t)(struct sched_prod *, void *arg);

void sched_prod_init(struct sched_prod *, int64_t scan_id);
enum sched_rc sched_prod_get_by_id(struct sched_prod *, int64_t id);
enum sched_rc sched_prod_add(struct sched_prod *);
enum sched_rc sched_prod_add_file(FILE *fp);

enum sched_rc sched_prod_write_begin(struct sched_prod const *,
                                     unsigned file_num);
enum sched_rc sched_prod_write_match(sched_prod_write_match_func_t *,
                                     void const *match, unsigned file_num);
enum sched_rc sched_prod_write_match_sep(unsigned file_num);
enum sched_rc sched_prod_write_end(unsigned file_num);

enum sched_rc sched_prod_get_all(sched_prod_set_func_t fn, struct sched_prod *,
                                 void *arg);

#endif
