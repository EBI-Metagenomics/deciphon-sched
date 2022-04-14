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

#define ACCESS "failed to access %s %s\n"
#define OPEN "failed to open %s for reading %s\n"
#define CALC_HASH "failed to compute hash of %s %s\n"
#define HASH_MISMATCH "hash mismatch for %s %s\n"

static void check_file(char const *name, char const *filename, int64_t xxh3,
                       void *arg)
{
    struct sched_health *health = arg;
    if (!xfile_exists(filename))
    {
        put(health, ACCESS, name, filename);
    }
    else
    {
        int64_t hash = 0;
        FILE *fp = fopen(filename, "rb");
        if (!fp)
        {
            put(health, OPEN, name, filename);
            return;
        }
        enum sched_rc rc = xfile_hash(fp, &hash);
        if (rc)
        {
            put(health, CALC_HASH, name, filename);
        }
        else
        {
            if (hash != xxh3)
            {
                put(health, HASH_MISMATCH, name, filename);
            }
        }
        fclose(fp);
    }
}

void health_check_db(struct sched_db *db, void *arg)
{
    check_file("database", db->filename, db->xxh3, arg);
}

void health_check_hmm(struct sched_hmm *hmm, void *arg)
{
    check_file("hmm", hmm->filename, hmm->xxh3, arg);
}
