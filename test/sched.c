#include "dcp_sched/sched.h"
#include "hope/hope.h"

void test_sched_reopen(void);
void test_sched_add_db(void);
void test_sched_submit_job(void);
void test_sched_submit_and_fetch_job(void);
void test_sched_submit_product(void);

int main(void)
{
    test_sched_reopen();
    test_sched_add_db();
    test_sched_submit_job();
    test_sched_submit_and_fetch_job();
    test_sched_submit_product();
    return hope_status();
}

void test_sched_reopen()
{
    remove(TMPDIR "/reopen.sched");
    EQ(sched_setup(TMPDIR "/reopen.sched"), SCHED_DONE);
    EQ(sched_open(), SCHED_DONE);
    EQ(sched_close(), SCHED_DONE);

    EQ(sched_open(), SCHED_DONE);
    EQ(sched_close(), SCHED_DONE);
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
    char const file1a[] = TMPDIR "/file1a.dcp";
    char const file1b[] = TMPDIR "/file1b.dcp";
    char const file1a_relative[] = TMPDIR "/dir/../file1a.dcp";
    char const file2[] = TMPDIR "/file2.dcp";
    char const file3[] = TMPDIR "/dir/does_not_exist.dcp";

    create_file1(file1a);
    create_file1(file1b);
    create_file2(file2);

    remove(sched_path);

    EQ(sched_setup(sched_path), SCHED_DONE);
    EQ(sched_open(), SCHED_DONE);

    int64_t db_id = 0;
    EQ(sched_add_db(file1a, &db_id), SCHED_DONE);
    EQ(db_id, 1);
    EQ(sched_add_db(file1a, &db_id), SCHED_DONE);
    EQ(db_id, 1);

    EQ(sched_add_db(file1b, &db_id), SCHED_FAIL);

    EQ(sched_add_db(file1a_relative, &db_id), SCHED_DONE);
    EQ(db_id, 1);

    EQ(sched_add_db(file2, &db_id), SCHED_DONE);
    EQ(db_id, 2);

    EQ(sched_add_db(file3, &db_id), SCHED_FAIL);

    EQ(sched_close(), SCHED_DONE);
}

void test_sched_submit_job(void)
{
    char const sched_path[] = TMPDIR "/submit_job.sched";
    char const db_path[] = TMPDIR "/submit_job.dcp";

    create_file1(db_path);
    remove(sched_path);

    EQ(sched_setup(sched_path), SCHED_DONE);
    EQ(sched_open(), SCHED_DONE);

    int64_t db_id = 0;
    EQ(sched_add_db(db_path, &db_id), SCHED_DONE);
    EQ(db_id, 1);

    EQ(sched_begin_job_submission(db_id, true, false), SCHED_DONE);
    sched_add_seq("seq0", "ACAAGCAG");
    sched_add_seq("seq1", "ACTTGCCG");
    EQ(sched_end_job_submission(), SCHED_DONE);

    EQ(sched_begin_job_submission(db_id, true, true), SCHED_DONE);
    sched_add_seq("seq0_2", "XXGG");
    sched_add_seq("seq1_2", "YXYX");
    EQ(sched_end_job_submission(), SCHED_DONE);

    EQ(sched_close(), SCHED_DONE);
}

void test_sched_submit_and_fetch_job()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_job.sched";
    char const db_path[] = TMPDIR "/submit_and_fetch_job.dcp";

    remove(sched_path);
    create_file1(db_path);

    EQ(sched_setup(sched_path), SCHED_DONE);
    EQ(sched_open(), SCHED_DONE);

    int64_t db_id = 0;
    EQ(sched_add_db(db_path, &db_id), SCHED_DONE);
    EQ(db_id, 1);

    EQ(sched_begin_job_submission(db_id, true, false), SCHED_DONE);
    sched_add_seq("seq0", "ACAAGCAG");
    sched_add_seq("seq1", "ACTTGCCG");
    EQ(sched_end_job_submission(), SCHED_DONE);

    EQ(sched_begin_job_submission(db_id, true, true), SCHED_DONE);
    sched_add_seq("seq0_2", "XXGG");
    sched_add_seq("seq1_2", "YXYX");
    EQ(sched_end_job_submission(), SCHED_DONE);

    int64_t job_id = 0;
    EQ(sched_next_pending_job(&job_id), SCHED_DONE);
    EQ(job_id, 1);
    EQ(sched_next_pending_job(&job_id), SCHED_DONE);
    EQ(job_id, 2);
    EQ(sched_next_pending_job(&job_id), SCHED_NOTFOUND);

    EQ(sched_close(), SCHED_DONE);
}

struct match
{
    char state[10];
    char codon[4];
};

static int write_match_cb(FILE *fp, void const *match)
{
    struct match const *m = match;
    if (fprintf(fp, "%s,", m->state) < 0) return SCHED_FAIL;
    if (fprintf(fp, "%s", m->codon) < 0) return SCHED_FAIL;
    return SCHED_DONE;
}

void test_sched_submit_product(void)
{
    char const sched_path[] = TMPDIR "/submit_product.sched";
    char const db_path[] = TMPDIR "/submit_product.dcp";

    remove(sched_path);
    create_file1(db_path);

    EQ(sched_setup(sched_path), SCHED_DONE);
    EQ(sched_open(), SCHED_DONE);

    int64_t db_id = 0;
    EQ(sched_add_db(db_path, &db_id), SCHED_DONE);
    EQ(db_id, 1);

    EQ(sched_begin_job_submission(db_id, true, false), SCHED_DONE);
    sched_add_seq("seq0", "ACAAGCAG");
    sched_add_seq("seq1", "ACTTGCCG");
    EQ(sched_end_job_submission(), SCHED_DONE);

    EQ(sched_begin_job_submission(db_id, true, true), SCHED_DONE);
    sched_add_seq("seq0_2", "XXGG");
    sched_add_seq("seq1_2", "YXYX");
    EQ(sched_end_job_submission(), SCHED_DONE);

    int64_t job_id = 0;
    EQ(sched_next_pending_job(&job_id), SCHED_DONE);

    EQ(sched_begin_prod_submission(), SCHED_DONE);

    struct sched_prod prod = {.id = 0,
                              .job_id = 0,
                              .seq_id = 1,
                              .match_id = 31,
                              .profile_name = "ACC0",
                              .abc_name = "dna",
                              .alt_loglik = -2720.381,
                              .null_loglik = -3163.185,
                              .profile_typeid = "protein",
                              .version = "0.0.4",
                              .match = 0};

    struct match match0 = {"state0", "GAC"};
    struct match match1 = {"state1", "GGC"};

    EQ(sched_prod_write_begin(&prod), SCHED_DONE);
    EQ(sched_prod_write_match(write_match_cb, &match0), SCHED_DONE);
    EQ(sched_prod_write_match_sep(), SCHED_DONE);
    EQ(sched_prod_write_match(write_match_cb, &match1), SCHED_DONE);
    EQ(sched_prod_write_end(), SCHED_DONE);

    prod.job_id = job_id;
    prod.seq_id = 2;
    prod.match_id = 39;

    strcpy(prod.profile_name, "ACC1");
    strcpy(prod.abc_name, "dna");

    prod.alt_loglik = -1111.;
    prod.null_loglik = -2222.;

    EQ(sched_prod_write_begin(&prod), SCHED_DONE);
    EQ(sched_prod_write_match(write_match_cb, &match0), SCHED_DONE);
    EQ(sched_prod_write_match_sep(), SCHED_DONE);
    EQ(sched_prod_write_match(write_match_cb, &match1), SCHED_DONE);
    EQ(sched_prod_write_end(), SCHED_DONE);

    EQ(sched_end_prod_submission(), SCHED_DONE);

    EQ(sched_close(), SCHED_DONE);
}
