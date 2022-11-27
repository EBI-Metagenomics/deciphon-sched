#include "sched/sched.h"
#include "fs.h"
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
static void test_submit_prodset(void);
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
    test_submit_prodset();
    test_wipe();
    return hope_status();
}

static void test_hmmer_filename(void)
{
    char filename[SCHED_FILENAME_SIZE] = {0};

    struct sched_hmmer_filename x = {1, 2, "profname"};
    sched_hmmer_filename_setup(&x, filename);
    eq(filename, "hmmer_1_2_profname.h3r");

    eq(sched_hmmer_filename_parse(&x, "hmmer_1_2_profname.h3r"), 0);
    eq(x.scan_id, 1);
    eq(x.seq_id, 2);
    eq(x.profile_name, "profname");
}

static void test_reopen()
{
    remove(TMPDIR "/reopen.sched");
    eq(sched_init(TMPDIR "/reopen.sched"), SCHED_OK);
    eq(sched_cleanup(), SCHED_OK);

    eq(sched_init(TMPDIR "/reopen.sched"), SCHED_OK);
    eq(sched_cleanup(), SCHED_OK);
}

static void create_file(char const *path, int seed)
{
    FILE *fp = fopen(path, "wb");
    notnull(fp);
    char const data[] = {
        0x64, 0x29, 0x20, 0x4e, 0x4f, 0x0a, 0x20, 0x20, 0x20,
        0x20, 0x69, 0x74, 0x73, 0x20, 0x49, 0x4e, 0x4f, 0x54,
        0x20, 0x4e, 0x20, 0x20, 0x68, 0x6d, 0x6d, 0x70, 0x61,
        0x74, 0x20, 0x49, 0x4e, 0x4f, 0x54, 0x20, 0x4e, 0x20,
        0x20, 0x73, 0x74, 0x61, 0x20, 0x43, 0x48, 0x45, (char)seed,
    };
    eq(fwrite(data, sizeof data, 1, fp), 1);
    eq(fclose(fp), 0);
}

static void test_submit_hmm(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const file1[] = "file1.hmm";
    char const file2[] = "file2.hmm";

    create_file(file1, 0);
    create_file(file2, 1);

    remove(sched_path);

    eq(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file1), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);

    eq(sched_hmm_set_file(&hmm, file2), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);

    eq(sched_cleanup(), SCHED_OK);
}

static void test_submit_hmm_nofile(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const nofile[] = TMPDIR "/file.hmm";

    create_file(nofile, 0);

    remove(sched_path);

    eq(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, nofile), SCHED_INVALID_FILE_NAME);

    eq(sched_cleanup(), SCHED_OK);
}

static void test_submit_hmm_same_file(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const file1a[] = "file1a.hmm";
    char const file1b[] = "file1b.hmm";

    create_file(file1a, 0);
    create_file(file1b, 0);

    remove(sched_path);

    eq(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file1a), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);

    eq(sched_hmm_set_file(&hmm, file1b), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_HMM_ALREADY_EXISTS);

    eq(sched_cleanup(), SCHED_OK);
}

static void test_submit_hmm_wrong_extension(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const file[] = "file.hm";

    create_file(file, 0);

    remove(sched_path);

    eq(sched_init(sched_path), SCHED_OK);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file), SCHED_INVALID_FILE_NAME_EXT);

    eq(sched_cleanup(), SCHED_OK);
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

    eq(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file1a_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file1a_dcp), SCHED_OK);
    eq(db.id, 1);

    eq(sched_hmm_set_file(&hmm, file1b_hmm), SCHED_OK);

    eq(sched_hmm_set_file(&hmm, file2_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file2_dcp), SCHED_OK);
    eq(db.id, 2);

    eq(sched_db_add(&db, file3_dcp), SCHED_ASSOC_HMM_NOT_FOUND);

    eq(sched_cleanup(), SCHED_OK);
}

static void test_submit_scan(void)
{
    char const sched_path[] = TMPDIR "/submit_job.sched";
    char const file_hmm[] = "submit_job.hmm";
    char const file_dcp[] = "submit_job.dcp";

    create_file(file_hmm, 0);
    create_file(file_dcp, 1);
    remove(sched_path);

    eq(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file_dcp), SCHED_OK);
    eq(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    eq(sched_cleanup(), SCHED_OK);
}

static void test_submit_and_fetch_scan_job()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_job.sched";
    char const file_hmm[] = "submit_and_fetch_job.hmm";
    char const file_dcp[] = "submit_and_fetch_job.dcp";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    eq(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(job.id, 1);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file_dcp), SCHED_OK);
    eq(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    eq(sched_job_next_pend(&job), SCHED_OK);
    eq(job.id, 2);
    eq(sched_job_set_run(job.id), SCHED_OK);

    eq(sched_job_next_pend(&job), SCHED_OK);
    eq(job.id, 3);
    eq(sched_job_set_run(job.id), SCHED_OK);

    eq(sched_job_next_pend(&job), SCHED_JOB_NOT_FOUND);

    eq(sched_cleanup(), SCHED_OK);
}

static void test_submit_and_fetch_seq()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_seq.sched";
    char const file_hmm[] = "submit_and_fetch_seq.hmm";
    char const file_dcp[] = "submit_and_fetch_seq.dcp";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    eq(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(job.id, 1);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file_dcp), SCHED_OK);
    eq(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    eq(sched_job_next_pend(&job), SCHED_OK);
    eq(job.id, 2);

    eq(sched_scan_get_by_job_id(&scan, job.id), SCHED_OK);

    sched_seq_init(&seq, 0, scan.id, "", "");
    eq(sched_seq_scan_next(&seq), SCHED_OK);
    eq(seq.id, 1);
    eq(seq.scan_id, 1);
    eq(seq.name, "seq0");
    eq(seq.data, "ACAAGCAG");
    eq(sched_seq_scan_next(&seq), SCHED_OK);
    eq(seq.id, 2);
    eq(seq.scan_id, 1);
    eq(seq.name, "seq1");
    eq(seq.data, "ACTTGCCG");
    eq(sched_seq_scan_next(&seq), SCHED_SEQ_NOT_FOUND);

    eq(sched_cleanup(), SCHED_OK);
}

static void callb(struct sched_prod *prod, struct sched_hmmer *hmmer, void *arg)
{
    (void)arg;
    static double evalue_logs[] = {-196.11220625901211, 0};
    static int lens[] = {5, 5};
    close(evalue_logs[prod->id - 1], prod->evalue_log);
    eq(lens[prod->id - 1], hmmer->len);
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

    eq(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(job.id, 1);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file_dcp), SCHED_OK);
    eq(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    eq(sched_job_next_pend(&job), SCHED_OK);
    eq(job.id, 2);

    eq(sched_scan_get_by_job_id(&scan, job.id), SCHED_OK);

    sched_seq_init(&seq, 0, scan.id, "", "");
    eq(sched_seq_scan_next(&seq), SCHED_OK);
    eq(seq.id, 1);
    eq(seq.scan_id, 1);
    eq(seq.name, "seq0");
    eq(seq.data, "ACAAGCAG");
    eq(sched_seq_scan_next(&seq), SCHED_OK);
    eq(seq.id, 2);
    eq(seq.scan_id, 1);
    eq(seq.name, "seq1");
    eq(seq.data, "ACTTGCCG");
    eq(sched_seq_scan_next(&seq), SCHED_SEQ_NOT_FOUND);

    eq(sched_prod_add_file(prod_path), 0);

    sched_hmmer_init(&hmmer, 1);
    eq(sched_hmmer_add(&hmmer, 5, (unsigned char const *)"hello"), 0);
    eq(hmmer.id, 1);

    sched_prod_init(&prod, 0);
    sched_hmmer_init(&hmmer, 0);
    eq(sched_prod_get_all(&callb, &prod, &hmmer, NULL), SCHED_HMMER_NOT_FOUND);

    sched_hmmer_init(&hmmer, 2);
    eq(sched_hmmer_add(&hmmer, 5, (unsigned char const *)"hello"), 0);
    eq(hmmer.id, 2);
    eq(sched_prod_get_all(&callb, &prod, &hmmer, NULL), 0);

    eq(sched_cleanup(), SCHED_OK);
}

static void file_write(char const *path, char const *str);

static void test_submit_prodset(void)
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_seq.sched";
    char const file_hmm[] = "submit_and_fetch_seq.hmm";
    char const file_dcp[] = "submit_and_fetch_seq.dcp";
    // char const prod_path[] = TESTDIR "/prod.tsv";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    eq(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(job.id, 1);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file_dcp), SCHED_OK);
    eq(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    eq(sched_job_next_pend(&job), SCHED_OK);
    eq(job.id, 2);

    eq(sched_scan_get_by_job_id(&scan, job.id), SCHED_OK);

    sched_seq_init(&seq, 0, scan.id, "", "");
    eq(sched_seq_scan_next(&seq), SCHED_OK);
    eq(seq.id, 1);
    eq(seq.scan_id, 1);
    eq(seq.name, "seq0");
    eq(seq.data, "ACAAGCAG");
    eq(sched_seq_scan_next(&seq), SCHED_OK);
    eq(seq.id, 2);
    eq(seq.scan_id, 1);
    eq(seq.name, "seq1");
    eq(seq.data, "ACTTGCCG");
    eq(sched_seq_scan_next(&seq), SCHED_SEQ_NOT_FOUND);

    char dir[256] = {0};
    eq(fs_mkdtemp(sizeof dir, dir), 0);

    char prod_path[256] = {0};
    strcpy(prod_path, dir);
    strcat(prod_path, "/prod.tsv");

    char hmmer_path0[256] = {0};
    strcpy(hmmer_path0, dir);
    strcat(hmmer_path0, "/hmmer_1_1_PF00742.20.h3r");

    char hmmer_path1[256] = {0};
    strcpy(hmmer_path1, dir);
    strcat(hmmer_path1, "/hmmer_1_2_PF00696.29.h3r");

    eq(fs_copy(prod_path, TESTDIR "/prod.tsv"), 0);
    file_write(hmmer_path0, "content0");
    file_write(hmmer_path1, "content1");

    sched_prodset_add(dir);
    eq(sched_cleanup(), SCHED_OK);
}

static void test_wipe(void)
{
    char const sched_path[] = TMPDIR "/wipe.sched";
    char const file_hmm[] = "wipe.hmm";
    char const file_dcp[] = "wipe.dcp";

    remove(sched_path);
    create_file(file_hmm, 0);
    create_file(file_dcp, 0);

    eq(sched_init(sched_path), SCHED_OK);

    sched_db_init(&db);
    sched_hmm_init(&hmm);

    eq(sched_hmm_set_file(&hmm, file_hmm), SCHED_OK);
    sched_job_init(&job, SCHED_HMM);
    eq(sched_job_submit(&job, &hmm), SCHED_OK);
    eq(job.id, 1);
    eq(sched_job_set_run(job.id), SCHED_OK);
    eq(sched_job_set_done(job.id), SCHED_OK);
    eq(sched_db_add(&db, file_dcp), SCHED_OK);
    eq(db.id, 1);

    sched_scan_init(&scan, db.id, true, false);
    sched_scan_add_seq("seq0", "ACAAGCAG");
    sched_scan_add_seq("seq1", "ACTTGCCG");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    sched_scan_init(&scan, db.id, true, true);
    sched_scan_add_seq("seq0_2", "XXGG");
    sched_scan_add_seq("seq1_2", "YXYX");

    sched_job_init(&job, SCHED_SCAN);
    eq(sched_job_submit(&job, &scan), SCHED_OK);

    eq(sched_wipe(), SCHED_OK);
    eq(sched_cleanup(), SCHED_OK);
}

static void file_write(char const *path, char const *str)
{
    FILE *fp = fopen(path, "wb");
    eq(fwrite(str, 1, strlen(str), fp), strlen(str));
    fclose(fp);
}
