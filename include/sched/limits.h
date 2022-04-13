#ifndef SCHED_LIMITS_H
#define SCHED_LIMITS_H

enum sched_limits
{
    ABC_NAME_SIZE = 16,
    FILENAME_SIZE = 128,
    JOB_ERROR_SIZE = 256,
    JOB_STATE_SIZE = 5,
    MATCH_SIZE = 5 * (1024 * 1024),
    MAX_NUM_THREADS = 64,
    NUM_SEQS_PER_JOB = 512,
    PATH_SIZE = 4096,
    PROFILE_NAME_SIZE = 64,
    PROFILE_TYPEID_SIZE = 16,
    SEQ_NAME_SIZE = 256,
    SEQ_SIZE = (1024 * 1024),
    VERSION_SIZE = 16,
};

#endif
