#include "sched/sched.h"
#include "hope/hope.h"

struct sched_db db = {0};
struct sched_job job = {0};
struct sched_scan scan = {0};
struct sched_seq seq = {0};

void test_sched_reopen(void);
void test_sched_add_db(void);
void test_sched_submit_scan_job(void);
void test_sched_submit_and_fetch_scan_job(void);
void test_sched_submit_and_fetch_seq(void);
void test_sched_wipe(void);

int main(void)
{
    test_sched_reopen();
    test_sched_add_db();
    test_sched_submit_scan_job();
    test_sched_submit_and_fetch_scan_job();
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

static void create_file(char const *path, int seed)
{
    FILE *fp = fopen(path, "wb");
    NOTNULL(fp);
    char const data[] = {
        0x64, 0x29, 0x20, 0x4e, 0x4f, 0x0a, 0x20, 0x20, 0x20,
        0x20, 0x69, 0x74, 0x73, 0x20, 0x49, 0x4e, 0x4f, 0x54,
        0x20, 0x4e, 0x20, 0x20, 0x68, 0x6d, 0x6d, 0x70, 0x61,
        0x74, 0x20, 0x49, 0x4e, 0x4f, 0x54, 0x20, 0x4e, 0x20,
        0x20, 0x73, 0x74, 0x61, 0x20, 0x43, 0x48, 0x45, (char)seed,
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

    create_file(nonfilename, 0);
    create_file(file1a, 0);
    create_file(file1b, 0);
    create_file(file2, 1);

    remove(sched_path);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    sched_db_init(&db);
    EQ(sched_db_add(&db, nonfilename), SCHED_EINVAL);
    EQ(sched_db_add(&db, file1a), SCHED_OK);
    EQ(db.id, 1);

    EQ(sched_db_add(&db, file1b), SCHED_EINVAL);

    EQ(sched_db_add(&db, file2), SCHED_OK);
    EQ(db.id, 2);

    EQ(sched_db_add(&db, file3), SCHED_EIO);

    EQ(sched_close(), SCHED_OK);
}

void test_sched_submit_scan_job(void)
{
    char const sched_path[] = TMPDIR "/submit_job.sched";
    char const db_filename[] = "submit_job.dcp";

    create_file(db_filename, 0);
    remove(sched_path);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    sched_db_init(&db);
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq(&scan, "seq0", "ACAAGCAG");
    sched_scan_add_seq(&scan, "seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq(&scan, "seq0_2", "XXGG");
    sched_scan_add_seq(&scan, "seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_close(), SCHED_OK);
}

void test_sched_submit_and_fetch_scan_job()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_job.sched";
    char const db_filename[] = "submit_and_fetch_job.dcp";

    remove(sched_path);
    create_file(db_filename, 0);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    sched_db_init(&db);
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq(&scan, "seq0", "ACAAGCAG");
    sched_scan_add_seq(&scan, "seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq(&scan, "seq0_2", "XXGG");
    sched_scan_add_seq(&scan, "seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

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
    create_file(db_filename, 0);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    sched_db_init(&db);
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq(&scan, "seq0", "ACAAGCAG");
    sched_scan_add_seq(&scan, "seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq(&scan, "seq0_2", "XXGG");
    sched_scan_add_seq(&scan, "seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 1);

    EQ(sched_scan_get_by_job_id(&scan, job.id), SCHED_OK);

    sched_seq_init(&seq, scan.id, "", "");
    EQ(sched_seq_next(&seq), SCHED_OK);
    EQ(seq.id, 1);
    EQ(seq.scan_id, 1);
    EQ(seq.name, "seq0");
    EQ(seq.data, "ACAAGCAG");
    EQ(sched_seq_next(&seq), SCHED_OK);
    EQ(seq.id, 2);
    EQ(seq.scan_id, 1);
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
    create_file(db_filename, 0);

    EQ(sched_setup(sched_path), SCHED_OK);
    EQ(sched_open(), SCHED_OK);

    sched_db_init(&db);
    EQ(sched_db_add(&db, db_filename), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq(&scan, "seq0", "ACAAGCAG");
    sched_scan_add_seq(&scan, "seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq(&scan, "seq0_2", "XXGG");
    sched_scan_add_seq(&scan, "seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_wipe(), SCHED_OK);
    EQ(sched_close(), SCHED_OK);
}
