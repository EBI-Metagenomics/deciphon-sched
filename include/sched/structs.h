#ifndef SCHED_STRUCTS_H
#define SCHED_STRUCTS_H

#include "sched/limits.h"
#include <stdint.h>

struct sched_scan
{
    int64_t id;
    int64_t db_id;

    int multi_hits;
    int hmmer3_compat;

    int64_t job_id;
};

struct sched_hmm
{
    int64_t id;
    int64_t xxh3;
    char filename[SCHED_FILENAME_SIZE];
    int64_t job_id;
};

struct sched_db
{
    int64_t id;
    int64_t xxh3;
    char filename[SCHED_FILENAME_SIZE];
    int64_t hmm_id;
};

struct sched_prod
{
    int64_t id;

    int64_t scan_id;
    int64_t seq_id;

    char profile_name[SCHED_PROFILE_NAME_SIZE];
    char abc_name[SCHED_ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[SCHED_PROFILE_TYPEID_SIZE];
    char version[SCHED_VERSION_SIZE];

    char match[SCHED_MATCH_SIZE];
};

enum sched_job_type
{
    SCHED_SCAN,
    SCHED_HMM
};

enum sched_job_state
{
    SCHED_PEND,
    SCHED_RUN,
    SCHED_DONE,
    SCHED_FAIL
};

struct sched_job
{
    int64_t id;
    int type;

    char state[SCHED_JOB_STATE_SIZE];
    int progress;
    char error[SCHED_JOB_ERROR_SIZE];

    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

struct sched_seq
{
    int64_t id;
    int64_t scan_id;
    char name[SCHED_SEQ_NAME_SIZE];
    char data[SCHED_SEQ_SIZE];
};

#endif
