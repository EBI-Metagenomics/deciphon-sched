#ifndef SCHED_LIMITS_H
#define SCHED_LIMITS_H

enum sched_limits
{
    SCHED_ABC_NAME_SIZE = 16,
    SCHED_FILENAME_SIZE = 128,
    SCHED_JOB_ERROR_SIZE = 256,
    SCHED_JOB_STATE_SIZE = 5,
    SCHED_MATCH_SIZE = 5 * (1024 * 1024),
    SCHED_MAX_NUM_THREADS = 64,
    SCHED_NUM_SEQS_PER_JOB = 512,
    SCHED_PATH_SIZE = 4096,
    SCHED_PROFILE_NAME_SIZE = 64,
    SCHED_PROFILE_TYPEID_SIZE = 16,
    SCHED_SEQ_NAME_SIZE = 256,
    SCHED_SEQ_SIZE = (1024 * 1024),
    SCHED_VERSION_SIZE = 16,
};

#endif
