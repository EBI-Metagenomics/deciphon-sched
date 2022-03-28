#include "sched_health.h"
#include "sched/sched.h"
#include "xfile.h"
#include <stdarg.h>

void put(struct sched_health *health, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(health->fp, fmt, args);
    health->num_errors += 1;
    va_end(args);
}

#define ACCESS(what) "failed to access " what " %s\n"
#define OPEN(what) "failed to open " what " for reading %s\n"
#define CALC_HASH(what) "failed to compute hash of " what " %s\n"
#define HASH_MISMATCH(what) "hash mismatch for " what " %s\n"

void health_check_db(struct sched_db *db, void *arg)
{
    struct sched_health *health = arg;
    if (!xfile_exists(db->filename))
    {
        put(health, ACCESS("database"), db->filename);
    }
    else
    {
        int64_t hash = 0;
        FILE *fp = fopen(db->filename, "rb");
        if (!fp)
        {
            put(health, OPEN("database"), db->filename);
        }
        enum sched_rc rc = xfile_hash(fp, &hash);
        if (rc)
        {
            put(health, CALC_HASH("database"), db->filename);
        }
        else
        {
            if (hash != db->xxh3)
            {
                put(health, HASH_MISMATCH("database"), db->filename);
            }
        }
        if (fp) fclose(fp);
    }
}

void health_check_hmm(struct sched_hmm *hmm, void *arg)
{
    struct sched_health *health = arg;
    if (!xfile_exists(hmm->filename))
    {
        put(health, ACCESS("hmm"), hmm->filename);
    }
    else
    {
        int64_t hash = 0;
        FILE *fp = fopen(hmm->filename, "rb");
        if (!fp)
        {
            put(health, OPEN("hmm"), hmm->filename);
        }
        enum sched_rc rc = xfile_hash(fp, &hash);
        if (rc)
        {
            put(health, CALC_HASH("hmm"), hmm->filename);
        }
        else
        {
            if (hash != hmm->xxh3)
            {
                put(health, HASH_MISMATCH("hmm"), hmm->filename);
            }
        }
        if (fp) fclose(fp);
    }
}
