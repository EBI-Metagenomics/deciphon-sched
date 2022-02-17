#include "sched/sched.h"
#include "hope/hope.h"

struct sched_job job = {0};
struct sched_seq seq = {0};
struct sched_prod prod = {0};

void test_sched_reopen(void);
void test_sched_add_db(void);
void test_sched_submit_job(void);
void test_sched_submit_and_fetch_job(void);
void test_sched_submit_and_fetch_seq(void);
// void test_sched_submit_product(void);
// void test_sched_submit_and_fetch_product(void);

int main(void)
{
    test_sched_reopen();
    test_sched_add_db();
    test_sched_submit_job();
    test_sched_submit_and_fetch_job();
    test_sched_submit_and_fetch_seq();
    // test_sched_submit_product();
    // test_sched_submit_and_fetch_product();
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

    struct sched_db db = {0};
    EQ(sched_db_add(&db, file1a), SCHED_DONE);
    EQ(db.id, 1);
    EQ(sched_db_add(&db, file1a), SCHED_DONE);
    EQ(db.id, 1);

    EQ(sched_db_add(&db, file1b), SCHED_EFAIL);

    EQ(sched_db_add(&db, file1a_relative), SCHED_DONE);
    EQ(db.id, 1);

    EQ(sched_db_add(&db, file2), SCHED_DONE);
    EQ(db.id, 2);

    EQ(sched_db_add(&db, file3), SCHED_EFAIL);

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

    struct sched_db db = {0};
    EQ(sched_db_add(&db, db_path), SCHED_DONE);
    EQ(db.id, 1);

    sched_job_init(&job, db.id, true, false);
    EQ(sched_job_begin_submission(&job), SCHED_DONE);
    sched_job_add_seq(&job, "seq0", "ACAAGCAG");
    sched_job_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_job_end_submission(&job), SCHED_DONE);

    sched_job_init(&job, db.id, true, true);
    EQ(sched_job_begin_submission(&job), SCHED_DONE);
    sched_job_add_seq(&job, "seq0_2", "XXGG");
    sched_job_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_job_end_submission(&job), SCHED_DONE);

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

    struct sched_db db = {0};
    EQ(sched_db_add(&db, db_path), SCHED_DONE);
    EQ(db.id, 1);

    sched_job_init(&job, db.id, true, false);
    EQ(sched_job_begin_submission(&job), SCHED_DONE);
    sched_job_add_seq(&job, "seq0", "ACAAGCAG");
    sched_job_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_job_end_submission(&job), SCHED_DONE);

    sched_job_init(&job, db.id, true, true);
    EQ(sched_job_begin_submission(&job), SCHED_DONE);
    sched_job_add_seq(&job, "seq0_2", "XXGG");
    sched_job_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_job_end_submission(&job), SCHED_DONE);

    EQ(sched_job_next_pend(&job), SCHED_DONE);
    EQ(job.id, 1);
    EQ(sched_job_next_pend(&job), SCHED_DONE);
    EQ(job.id, 2);
    EQ(sched_job_next_pend(&job), SCHED_NOTFOUND);

    EQ(sched_close(), SCHED_DONE);
}

void test_sched_submit_and_fetch_seq()
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_seq.sched";
    char const db_path[] = TMPDIR "/submit_and_fetch_seq.dcp";

    remove(sched_path);
    create_file1(db_path);

    EQ(sched_setup(sched_path), SCHED_DONE);
    EQ(sched_open(), SCHED_DONE);

    struct sched_db db = {0};
    EQ(sched_db_add(&db, db_path), SCHED_DONE);
    EQ(db.id, 1);

    sched_job_init(&job, db.id, true, false);
    EQ(sched_job_begin_submission(&job), SCHED_DONE);
    sched_job_add_seq(&job, "seq0", "ACAAGCAG");
    sched_job_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_job_end_submission(&job), SCHED_DONE);

    sched_job_init(&job, db.id, true, true);
    EQ(sched_job_begin_submission(&job), SCHED_DONE);
    sched_job_add_seq(&job, "seq0_2", "XXGG");
    sched_job_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_job_end_submission(&job), SCHED_DONE);

    EQ(sched_job_next_pend(&job), SCHED_DONE);
    EQ(job.id, 1);

    sched_seq_init(&seq, job.id, "", "");
    EQ(sched_seq_next(&seq), SCHED_DONE);
    EQ(seq.id, 1);
    EQ(seq.job_id, 1);
    EQ(seq.name, "seq0");
    EQ(seq.data, "ACAAGCAG");
    EQ(sched_seq_next(&seq), SCHED_DONE);
    EQ(seq.id, 2);
    EQ(seq.job_id, 1);
    EQ(seq.name, "seq1");
    EQ(seq.data, "ACTTGCCG");
    EQ(sched_seq_next(&seq), SCHED_DONE);

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
    if (fprintf(fp, "%s,", m->state) < 0) return SCHED_EFAIL;
    if (fprintf(fp, "%s", m->codon) < 0) return SCHED_EFAIL;
    return SCHED_DONE;
}

// void test_sched_submit_product(void)
// {
//     char const sched_path[] = TMPDIR "/submit_product.sched";
//     char const db_path[] = TMPDIR "/submit_product.dcp";
//
//     remove(sched_path);
//     create_file1(db_path);
//
//     EQ(sched_setup(sched_path), SCHED_DONE);
//     EQ(sched_open(), SCHED_DONE);
//
//     struct sched_db db = {0};
//     EQ(sched_db_add(&db, db_path), SCHED_DONE);
//     EQ(db.id, 1);
//
//     sched_job_init(&job, db.id, true, false);
//     EQ(sched_job_begin_submission(&job), SCHED_DONE);
//     sched_job_add_seq(&job, "seq0", "ACAAGCAG");
//     sched_job_add_seq(&job, "seq1", "ACTTGCCG");
//     EQ(sched_job_end_submission(&job), SCHED_DONE);
//
//     sched_job_init(&job, db.id, true, true);
//     EQ(sched_job_begin_submission(&job), SCHED_DONE);
//     sched_job_add_seq(&job, "seq0_2", "XXGG");
//     sched_job_add_seq(&job, "seq1_2", "YXYX");
//     EQ(sched_job_end_submission(&job), SCHED_DONE);
//
//     EQ(sched_job_next_pend(&job), SCHED_DONE);
//
//     EQ(sched_begin_prod_submission(1), SCHED_DONE);
//
//     prod.id = 0;
//     prod.job_id = job.id;
//     prod.seq_id = 1;
//     strcpy(prod.profile_name, "ACC0");
//     strcpy(prod.abc_name, "dna");
//     prod.alt_loglik = -2720.381;
//     prod.null_loglik = -3163.185;
//     strcpy(prod.profile_typeid, "protein");
//     strcpy(prod.version, "0.0.4");
//     strcpy(prod.match, "A,B,C");
//
//     struct match match0 = {"state0", "GAC"};
//     struct match match1 = {"state1", "GGC"};
//
//     EQ(sched_prod_write_begin(&prod, 0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match0, 0), SCHED_DONE);
//     EQ(sched_prod_write_match_sep(0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match1, 0), SCHED_DONE);
//     EQ(sched_prod_write_end(0), SCHED_DONE);
//
//     prod.job_id = job.id;
//     prod.seq_id = 2;
//
//     strcpy(prod.profile_name, "ACC1");
//     strcpy(prod.abc_name, "dna");
//
//     prod.alt_loglik = -1111.;
//     prod.null_loglik = -2222.;
//
//     EQ(sched_prod_write_begin(&prod, 0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match0, 0), SCHED_DONE);
//     EQ(sched_prod_write_match_sep(0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match1, 0), SCHED_DONE);
//     EQ(sched_prod_write_end(0), SCHED_DONE);
//
//     EQ(sched_end_prod_submission(), SCHED_DONE);
//
//     EQ(sched_close(), SCHED_DONE);
// }
//
// void test_sched_submit_and_fetch_product(void)
// {
//     char const sched_path[] = TMPDIR "/submit_and_fetch_product.sched";
//     char const db_path[] = TMPDIR "/submit_and_fetch_product.dcp";
//
//     remove(sched_path);
//     create_file1(db_path);
//
//     EQ(sched_setup(sched_path), SCHED_DONE);
//     EQ(sched_open(), SCHED_DONE);
//
//     struct sched_db db = {0};
//     EQ(sched_db_add(&db, db_path), SCHED_DONE);
//     EQ(db.id, 1);
//
//     sched_job_init(&job, db.id, true, false);
//     EQ(sched_job_begin_submission(&job), SCHED_DONE);
//     sched_job_add_seq(&job, "seq0", "ACAAGCAG");
//     sched_job_add_seq(&job, "seq1", "ACTTGCCG");
//     EQ(sched_job_end_submission(&job), SCHED_DONE);
//
//     sched_job_init(&job, db.id, true, true);
//     EQ(sched_job_begin_submission(&job), SCHED_DONE);
//     sched_job_add_seq(&job, "seq0_2", "XXGG");
//     sched_job_add_seq(&job, "seq1_2", "YXYX");
//     EQ(sched_job_end_submission(&job), SCHED_DONE);
//
//     EQ(sched_job_next_pend(&job), SCHED_DONE);
//
//     EQ(sched_job_begin_submission(&job), SCHED_DONE);
//     EQ(sched_rollback_job_submission(&job), SCHED_DONE);
//
//     EQ(sched_begin_prod_submission(1), SCHED_DONE);
//
//     prod.id = 0;
//     prod.job_id = job.id;
//     prod.seq_id = 1;
//     strcpy(prod.profile_name, "ACC0");
//     strcpy(prod.abc_name, "dna");
//     prod.alt_loglik = -2720.381;
//     prod.null_loglik = -3163.185;
//     strcpy(prod.profile_typeid, "protein");
//     strcpy(prod.version, "0.0.4");
//
//     struct match match0 = {"state0", "GAC"};
//     struct match match1 = {"state1", "GGC"};
//
//     EQ(sched_prod_write_begin(&prod, 0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match0, 0), SCHED_DONE);
//     EQ(sched_prod_write_match_sep(0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match1, 0), SCHED_DONE);
//     EQ(sched_prod_write_end(0), SCHED_DONE);
//
//     prod.job_id = job.id;
//     prod.seq_id = 2;
//
//     strcpy(prod.profile_name, "ACC1");
//     strcpy(prod.abc_name, "dna");
//
//     prod.alt_loglik = -1111.;
//     prod.null_loglik = -2222.;
//
//     EQ(sched_prod_write_begin(&prod, 0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match0, 0), SCHED_DONE);
//     EQ(sched_prod_write_match_sep(0), SCHED_DONE);
//     EQ(sched_prod_write_match(write_match_cb, &match1, 0), SCHED_DONE);
//     EQ(sched_prod_write_end(0), SCHED_DONE);
//
//     EQ(sched_end_prod_submission(), SCHED_DONE);
//
//     enum sched_job_state state = 0;
//
//     EQ(sched_set_job_done(1), SCHED_DONE);
//     EQ(sched_job_state(1, &state), SCHED_DONE);
//     EQ(state, SCHED_JOB_DONE);
//
//     EQ(sched_set_job_fail(2, "error msg"), SCHED_DONE);
//     EQ(sched_job_state(2, &state), SCHED_DONE);
//     EQ(state, SCHED_JOB_FAIL);
//
//     sched_prod_init(&prod, 1);
//     EQ(sched_prod_next(&prod), SCHED_NEXT);
//     EQ(prod.id, 1);
//     EQ(prod.job_id, 1);
//     EQ(prod.match, "state0,GAC;state1,GGC");
//
//     EQ(sched_close(), SCHED_DONE);
// }
