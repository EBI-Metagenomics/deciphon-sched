#include "sched/sched.h"
#include "hope.h"

struct sched_hmm hmm = {0};
struct sched_db db = {0};
struct sched_job job = {0};
struct sched_scan scan = {0};
struct sched_seq seq = {0};
struct sched_prod prod = {0};
struct sched_hmmer hmmer = {0};

static void test_hmmer_filename(void);
static void test_reopen(void);
static void test_submit_hmm(void);
static void test_submit_hmm_nofile(void);
static void test_submit_hmm_same_file(void);
static void test_submit_hmm_wrong_extension(void);
static void test_add_db(void);
static void test_submit_scan(void);
static void test_submit_and_fetch_scan_job(void);
static void test_submit_and_fetch_seq(void);
static void test_submit_prod(void);
static void test_wipe(void);

int main(void)
{
    test_hmmer_filename();
    test_reopen();
    test_submit_hmm();
    test_submit_hmm_nofile();
    test_submit_hmm_same_file();
    test_submit_hmm_wrong_extension();
    test_add_db();
    test_submit_scan();
    test_submit_and_fetch_scan_job();
    test_submit_and_fetch_seq();
    test_submit_prod();
    test_wipe();
    return hope_status();
}

static void test_hmmer_filename(void)
{
    char filename[SCHED_FILENAME_SIZE] = {0};

    struct sched_hmmer_filename x = {1, 2, "profname"};
    sched_hmmer_filename_setup(&x, filename);
    EQ(filename, "hmmer_1_2_profname.h3r");

    EQ(sched_hmmer_filename_parse(&x, "hmmer_1_2_profname.h3r"), 0);
    EQ(x.scan_id, 1);
    EQ(x.seq_id, 2);
    EQ(x.profile_name, "profname");
}

static void test_reopen()
{
    remove(TMPDIR "/reopen.sched");
    EQ(sched_init(TMPDIR "/reopen.sched"), SCHED_OK);
    EQ(sched_cleanup(), SCHED_OK);

    EQ(sched_init(TMPDIR "/reopen.sched"), SCHED_OK);
    EQ(sched_cleanup(), SCHED_OK);
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

static void test_submit_hmm(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const file1[] = "file1.hmm";
    char const file2[] = "file2.hmm";

    create_file(file1, 0);
    create_file(file2, 1);

    remove(sched_path);

    EQ(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file1), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);

    EQ(sched_hmm_set_file(&hmm, file2), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_submit_hmm_nofile(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const nofile[] = TMPDIR "/file.hmm";

    create_file(nofile, 0);

    remove(sched_path);

    EQ(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, nofile), SCHED_INVALID_FILE_NAME);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_submit_hmm_same_file(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const file1a[] = "file1a.hmm";
    char const file1b[] = "file1b.hmm";

    create_file(file1a, 0);
    create_file(file1b, 0);

    remove(sched_path);

    EQ(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file1a), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);

    EQ(sched_hmm_set_file(&hmm, file1b), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_HMM_ALREADY_EXISTS);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_submit_hmm_wrong_extension(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const file[] = "file.hm";

    create_file(file, 0);

    remove(sched_path);

    EQ(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file), SCHED_INVALID_FILE_NAME_EXT);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_add_db(void)
{
    char const sched_path[] = TMPDIR "/file.sched";

    char const nofile_hmm[] = TMPDIR "/file1a.hmm";
    char const file1a_hmm[] = "file1a.hmm";
    char const file1b_hmm[] = "file1b.hmm";
    char const file2_hmm[] = "file2.hmm";

    create_file(nofile_hmm, 0);
    create_file(file1a_hmm, 0);
    create_file(file1b_hmm, 0);
    create_file(file2_hmm, 1);

    char const nofile_dcp[] = TMPDIR "/file1a.dcp";
    char const file1a_dcp[] = "file1a.dcp";
    char const file1b_dcp[] = "file1b.dcp";
    char const file2_dcp[] = "file2.dcp";
    char const file3_dcp[] = "does_not_exist.dcp";

    create_file(nofile_dcp, 0);
    create_file(file1a_dcp, 0);
    create_file(file1b_dcp, 0);
    create_file(file2_dcp, 1);

    create_file(nofile_dcp, 0);
    create_file(file1a_dcp, 0);
    create_file(file1b_dcp, 0);
    create_file(file2_dcp, 1);

    remove(sched_path);

    EQ(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file1a_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);
    EQ(sched_job_set_run(job.id), SCHED_OK);
    EQ(sched_job_set_done(job.id), SCHED_OK);
    EQ(sched_db_add(&db, file1a_dcp), SCHED_OK);
    EQ(db.id, 1);

    EQ(sched_hmm_set_file(&hmm, file1b_hmm), SCHED_OK);

    EQ(sched_hmm_set_file(&hmm, file2_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);
    EQ(sched_job_set_run(job.id), SCHED_OK);
    EQ(sched_job_set_done(job.id), SCHED_OK);
    EQ(sched_db_add(&db, file2_dcp), SCHED_OK);
    EQ(db.id, 2);

    EQ(sched_db_add(&db, file3_dcp), SCHED_ASSOC_HMM_NOT_FOUND);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_submit_scan(void)
{
    char const sched_path[] = TMPDIR "/submit_job.sched";
    char const file_hmm[] = "submit_job.hmm";
    char const file_dcp[] = "submit_job.dcp";

    create_file(file_hmm, 0);
    create_file(file_dcp, 1);
    remove(sched_path);

    EQ(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);
    EQ(sched_job_set_run(job.id), SCHED_OK);
    EQ(sched_job_set_done(job.id), SCHED_OK);
    EQ(sched_db_add(&db, file_dcp), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_submit_and_fetch_scan_job()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_job.sched";
    char const file_hmm[] = "submit_and_fetch_job.hmm";
    char const file_dcp[] = "submit_and_fetch_job.dcp";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    EQ(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);
    EQ(job.id, 1);
    EQ(sched_job_set_run(job.id), SCHED_OK);
    EQ(sched_job_set_done(job.id), SCHED_OK);
    EQ(sched_db_add(&db, file_dcp), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 2);
    EQ(sched_job_set_run(job.id), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 3);
    EQ(sched_job_set_run(job.id), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_JOB_NOT_FOUND);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_submit_and_fetch_seq()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_seq.sched";
    char const file_hmm[] = "submit_and_fetch_seq.hmm";
    char const file_dcp[] = "submit_and_fetch_seq.dcp";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    EQ(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);
    EQ(job.id, 1);
    EQ(sched_job_set_run(job.id), SCHED_OK);
    EQ(sched_job_set_done(job.id), SCHED_OK);
    EQ(sched_db_add(&db, file_dcp), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 2);

    EQ(sched_scan_get_by_job_id(&scan, job.id), SCHED_OK);

    sched_seq_init(&seq, 0, scan.id, "", "");
    EQ(sched_seq_scan_next(&seq), SCHED_OK);
    EQ(seq.id, 1);
    EQ(seq.scan_id, 1);
    EQ(seq.name, "seq0");
    EQ(seq.data, "ACAAGCAG");
    EQ(sched_seq_scan_next(&seq), SCHED_OK);
    EQ(seq.id, 2);
    EQ(seq.scan_id, 1);
    EQ(seq.name, "seq1");
    EQ(seq.data, "ACTTGCCG");
    EQ(sched_seq_scan_next(&seq), SCHED_SEQ_NOT_FOUND);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_submit_prod(void)
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_seq.sched";
    char const file_hmm[] = "submit_and_fetch_seq.hmm";
    char const file_dcp[] = "submit_and_fetch_seq.dcp";
    char const prod_path[] = TESTDIR "/prod.tsv";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    EQ(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);
    EQ(job.id, 1);
    EQ(sched_job_set_run(job.id), SCHED_OK);
    EQ(sched_job_set_done(job.id), SCHED_OK);
    EQ(sched_db_add(&db, file_dcp), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_job_next_pend(&job), SCHED_OK);
    EQ(job.id, 2);

    EQ(sched_scan_get_by_job_id(&scan, job.id), SCHED_OK);

    sched_seq_init(&seq, 0, scan.id, "", "");
    EQ(sched_seq_scan_next(&seq), SCHED_OK);
    EQ(seq.id, 1);
    EQ(seq.scan_id, 1);
    EQ(seq.name, "seq0");
    EQ(seq.data, "ACAAGCAG");
    EQ(sched_seq_scan_next(&seq), SCHED_OK);
    EQ(seq.id, 2);
    EQ(seq.scan_id, 1);
    EQ(seq.name, "seq1");
    EQ(seq.data, "ACTTGCCG");
    EQ(sched_seq_scan_next(&seq), SCHED_SEQ_NOT_FOUND);

    sched_prod_init(&prod, 1);
    EQ(sched_prod_add_file(prod_path), 0);

    sched_hmmer_init(&hmmer, prod.id);
    char const hello[] = "hello";
    EQ(sched_hmmer_add(&hmmer, 5, (unsigned char const *)hello), 0);

    EQ(sched_cleanup(), SCHED_OK);
}

static void test_wipe(void)
{
    char const sched_path[] = TMPDIR "/wipe.sched";
    char const file_hmm[] = "wipe.hmm";
    char const file_dcp[] = "wipe.dcp";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    EQ(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    EQ(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    EQ(sched_job_submit(&job, &hmm), SCHED_OK);
    EQ(job.id, 1);
    EQ(sched_job_set_run(job.id), SCHED_OK);
    EQ(sched_job_set_done(job.id), SCHED_OK);
    EQ(sched_db_add(&db, file_dcp), SCHED_OK);
    EQ(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    EQ(sched_job_submit(&job, &scan), SCHED_OK);

    EQ(sched_wipe(), SCHED_OK);
    EQ(sched_cleanup(), SCHED_OK);
}
