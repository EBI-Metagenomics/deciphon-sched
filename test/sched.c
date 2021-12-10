#include "dcp_sched/sched.h"
#include "hope/hope.h"

void test_sched_reopen(void);
void test_sched_add_db(void);
void test_sched_submit_job(void);
void test_sched_submit_and_fetch_job(void);

int main(void)
{
    test_sched_reopen();
    test_sched_add_db();
    /* test_sched_submit_job(); */
    /* test_sched_submit_and_fetch_job(1); */
    /* test_sched_submit_and_fetch_job(4); */
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

static void create_file(char const *path)
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

void test_sched_add_db(void)
{
    char const sched_path[] = TMPDIR "/file.sched";
    char const db_path[] = TMPDIR "/file.dcp";
    create_file(db_path);

    remove(sched_path);

    EQ(sched_setup(sched_path), SCHED_DONE);
    EQ(sched_open(), SCHED_DONE);

    int64_t db_id = 0;
    EQ(sched_add_db(db_path, &db_id), SCHED_DONE);
    EQ(sched_add_db(db_path, &db_id), SCHED_DONE);

    EQ(sched_close(), SCHED_DONE);
}

/* void test_sched_submit_job(void) */
/* { */
/*     char const sched_path[] = TMPDIR "/submit_job.sched"; */
/*     char const db_path[] = TMPDIR "/standard_example1.dcp"; */
/*     remove(sched_path); */
/*  */
/*     EQ(server_open(sched_path, 1), SCHED_DONE); */
/*  */
/*     standard_db_examples_new_ex1(db_path); */
/*     int64_t db_id = 0; */
/*     EQ(server_add_db(db_path, &db_id), SCHED_DONE); */
/*     EQ(db_id, 1); */
/*  */
/*     struct job job = {0}; */
/*     job_init(&job, db_id); */
/*     struct seq seq[2] = {0}; */
/*     seq_init(seq + 0, "seq0", imm_str(imm_example1_seq)); */
/*     seq_init(seq + 1, "seq1", imm_str(imm_example2_seq)); */
/*     job_add_seq(&job, seq + 0); */
/*     job_add_seq(&job, seq + 1); */
/*  */
/*     EQ(server_submit_job(&job), SCHED_DONE); */
/*     EQ(job.id, 1); */
/*  */
/*     EQ(server_close(), SCHED_DONE); */
/* } */
/*  */
/* void test_sched_submit_and_fetch_job(unsigned num_threads) */
/* { */
/*     char const sched_path[] = TMPDIR "/submit_and_fetch_job.sched"; */
/*     char const db_path[] = TMPDIR "/protein_example1.dcp"; */
/*     remove(sched_path); */
/*  */
/*     EQ(server_open(sched_path, num_threads), SCHED_DONE); */
/*  */
/*     protein_db_examples_new_ex1(db_path); */
/*     int64_t db_id = 0; */
/*     EQ(server_add_db(db_path, &db_id), SCHED_DONE); */
/*     EQ(db_id, 1); */
/*  */
/*     struct job job = {0}; */
/*     job_init(&job, db_id); */
/*     struct seq seq[2] = {0}; */
/*     seq_init(seq + 0, "seq0", imm_str(imm_example1_seq)); */
/*     seq_init(seq + 1, "seq1", imm_str(imm_example2_seq)); */
/*     job_add_seq(&job, seq + 0); */
/*     job_add_seq(&job, seq + 1); */
/*  */
/*     EQ(server_submit_job(&job), SCHED_DONE); */
/*     EQ(job.id, 1); */
/*  */
/*     enum job_state state = 0; */
/*     EQ(server_job_state(job.id, &state), SCHED_DONE); */
/*     EQ(state, JOB_PEND); */
/*  */
/*     EQ(server_job_state(2, &state), RC_NOTFOUND); */
/*  */
/*     EQ(server_run(true), SCHED_DONE); */
/*  */
/*     int64_t prod_id = 0; */
/*     EQ(server_next_prod(1, &prod_id), RC_NEXT); */
/*     EQ(prod_id, 1); */
/*  */
/*     struct prod const *p = server_get_prod(); */
/*  */
/*     EQ(p->id, 1); */
/*     EQ(p->seq_id, 2); */
/*     EQ(p->match_id, 1); */
/*     EQ(p->prof_name, "ACC0"); */
/*     EQ(p->abc_name, "dna_iupac"); */
/*     CLOSE(p->loglik, -2720.38134765625); */
/*     CLOSE(p->null_loglik, -3163.185302734375); */
/*     EQ(p->prof_typeid, "protein"); */
/*     EQ(p->version, "0.0.4"); */
/*  */
/*     extern char const prod1_match_data[]; */
/*     for (unsigned i = 0; i < array_size(p->match); ++i) */
/*     { */
/*         EQ(array_data(p->match)[i], prod1_match_data[i]); */
/*     } */
/*  */
/*     EQ(server_next_prod(1, &prod_id), RC_NEXT); */
/*     EQ(prod_id, 2); */
/*  */
/*     EQ(p->id, 2); */
/*     EQ(p->seq_id, 2); */
/*     EQ(p->match_id, 2); */
/*     EQ(p->prof_name, "ACC1"); */
/*     EQ(p->abc_name, "dna_iupac"); */
/*     CLOSE(p->loglik, -2854.53369140625); */
/*     CLOSE(p->null_loglik, -3094.66308593750); */
/*     EQ(p->prof_typeid, "protein"); */
/*     EQ(p->version, "0.0.4"); */
/*  */
/*     EQ(server_next_prod(1, &prod_id), SCHED_DONE); */
/*  */
/*     EQ(server_close(), SCHED_DONE); */
/* } */
