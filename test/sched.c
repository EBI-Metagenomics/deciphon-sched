#include "sched/sched.h"
#include "hope/hope.h"

struct sched_job job = {0};
struct sched_seq seq = {0};

void test_sched_reopen(void);
void test_sched_add_db(void);
void test_sched_submit_job(void);
void test_sched_submit_and_fetch_job(void);
void test_sched_submit_and_fetch_seq(void);
void test_sched_wipe(void);

static void default_print(char const *msg, void *arg)
{
    (void)arg;
    fprintf(stderr, "%s\n", msg);
}

int main(void)
{
    sched_logger_setup(default_print, 0);
    test_sched_reopen();
    test_sched_add_db();
    test_sched_submit_job();
    test_sched_submit_and_fetch_job();
    test_sched_submit_and_fetch_seq();
    test_sched_wipe();
    return hope_status();
}

void test_sched_reopen()
{
    remove(TMPDIR "/reopen.sched");
    EQ(sched_setup(TMPDIR "/reopen.sched"), SCHED_OK);
    EQ(sched_open(), SCHED_OK);
    EQ(sched_close(), SCHED_OK);

    EQ(sched_open(), SCHED_OK);
    EQ(sched_close(), SCHED_OK);
}

static void create_file1(char const *path)
{
    FILE *fp = fopen(path, "wb");
    NOTNULL(fp);
    char const data[] = {
        0x64, 0x29, 0x20, 0x4e, 0x4f, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x69, 0x74,
        0x73, 0x20, 0x49, 0x4e, 0x4f, 0x54, 0x20, 0x4e, 0x20, 0x20, 0x68, 0x6d,
        0x6d, 0x70, 0x61, 0x74, 0x20, 0x49, 0x4e, 0x4f, 0x54, 0x20, 0x4e, 0x20,
        0x20, 0x73, 0x74, 0x61, 0x20, 0x43, 0x48, 0x45, 0x43,
    };
    EQ(fwrite(data, sizeof data, 1, fp), 1);
    EQ(fclose(fp), 0);
}

static void create_file2(char const *path)
{
    FILE *fp = fopen(path, "wb");
    NOTNULL(fp);
    char const data[] = {
        0x64, 0x29, 0x20, 0x4e, 0x4f, 0x0a, 0x20, 0x20, 0x20, 0x20,
        0x69, 0x74, 0x73, 0x54, 0x20, 0x4e, 0x20, 0x20, 0x68, 0x6d,
        0x6d, 0x49, 0x4e, 0x4f, 0x54, 0x20, 0x4e, 0x20, 0x20, 0x73,
        0x74, 0x61, 0x20, 0x43, 0x48, 0x45, 0x43,
    };
    EQ(fwrite(data, sizeof data, 1, fp), 1);
    EQ(fclose(fp), 0);
}

void test_sched_add_db(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const nonfilename[] = TMPDIR "/file1a.dcp";
    char const file1a[] = "file1a.dcp";
    char const file1b[] = "file1b.dcp";
    char const file2[] = "file2.dcp";
    char const file3[] = "does_not_exist.dcp";

    create_file1(nonfilename);
    create_file1(file1a);
    create_file1(file1b);
    create_file2(file2);

    remove(sched_path);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    struct sched_db db = {0};
    EQ(sched_db_add(&db, nonfilename), SCHED_EINVAL);
    EQ(sched_db_add(&db, file1a), SCHED_OK);
    EQ(db.id, 1);

    EQ(sched_db_add(&db, file1b), SCHED_EINVAL);

    EQ(sched_db_add(&db, file2), SCHED_OK);
    EQ(db.id, 2);

    EQ(sched_db_add(&db, file3), SCHED_EIO);

    EQ(sched_close(), SCHED_OK);
}

void test_sched_submit_job(void)
{
    char const sched_path[] = TMPDIR "/submit_job.sched";
    char const db_filename[] = "submit_job.dcp";

    create_file1(db_filename);
    remove(sched_path);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    struct sched_db db = {0};
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_job_init(&job, db.id, true, false);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0", "ACAAGCAG");
    sched_job_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    sched_job_init(&job, db.id, true, true);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0_2", "XXGG");
    sched_job_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    EQ(sched_close(), SCHED_OK);
}

void test_sched_submit_and_fetch_job()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_job.sched";
    char const db_filename[] = "submit_and_fetch_job.dcp";

    remove(sched_path);
    create_file1(db_filename);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    struct sched_db db = {0};
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_job_init(&job, db.id, true, false);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0", "ACAAGCAG");
    sched_job_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    sched_job_init(&job, db.id, true, true);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0_2", "XXGG");
    sched_job_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 1);
    EQ(sched_job_set_run(job.id), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 2);
    EQ(sched_job_set_run(job.id), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_NOTFOUND);

    EQ(sched_close(), SCHED_OK);
}

void test_sched_submit_and_fetch_seq()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_seq.sched";
    char const db_filename[] = "submit_and_fetch_seq.dcp";

    remove(sched_path);
    create_file1(db_filename);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    struct sched_db db = {0};
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_job_init(&job, db.id, true, false);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0", "ACAAGCAG");
    sched_job_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    sched_job_init(&job, db.id, true, true);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0_2", "XXGG");
    sched_job_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 1);

    sched_seq_init(&seq, job.id, "", "");
    EQ(sched_seq_next(&seq), SCHED_OK);
    EQ(seq.id, 1);
    EQ(seq.job_id, 1);
    EQ(seq.name, "seq0");
    EQ(seq.data, "ACAAGCAG");
    EQ(sched_seq_next(&seq), SCHED_OK);
    EQ(seq.id, 2);
    EQ(seq.job_id, 1);
    EQ(seq.name, "seq1");
    EQ(seq.data, "ACTTGCCG");
    EQ(sched_seq_next(&seq), SCHED_END);

    EQ(sched_close(), SCHED_OK);
}

void test_sched_wipe(void)
{
    char const sched_path[] = TMPDIR "/wipe.sched";
    char const db_filename[] = "wipe.dcp";

    remove(sched_path);
    create_file1(db_filename);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    struct sched_db db = {0};
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_job_init(&job, db.id, true, false);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0", "ACAAGCAG");
    sched_job_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    sched_job_init(&job, db.id, true, true);
    EQ(sched_job_begin_submission(&job), SCHED_OK);
    sched_job_add_seq(&job, "seq0_2", "XXGG");
    sched_job_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_job_end_submission(&job), SCHED_OK);

    EQ(sched_wipe(), SCHED_OK);
    EQ(sched_close(), SCHED_OK);
}
