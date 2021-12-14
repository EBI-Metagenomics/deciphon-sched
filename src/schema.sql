PRAGMA foreign_keys = off;

BEGIN TRANSACTION;

CREATE TABLE job (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,

    db_id INTEGER REFERENCES db (id) NOT NULL,
    multi_hits INTEGER NOT NULL,
    hmmer3_compat INTEGER NOT NULL,
    state TEXT CHECK(state IN ('pend', 'run', 'done', 'fail')) NOT NULL,

    error TEXT NOT NULL,
    submission INTEGER NOT NULL,
    exec_started INTEGER NOT NULL,
    exec_ended INTEGER NOT NULL
);

CREATE TABLE seq (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,
    job_id INTEGER REFERENCES job (id) NOT NULL,
    name TEXT NOT NULL,
    data TEXT NOT NULL
);

CREATE TABLE prod (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,

    job_id INTEGER REFERENCES job (id) NOT NULL,
    seq_id INTEGER REFERENCES seq (id) NOT NULL,

    profile_name TEXT NOT NULL,
    abc_name TEXT NOT NULL,

    alt_loglik REAL NOT NULL,
    null_loglik REAL NOT NULL,

    profile_typeid TEXT NOT NULL,
    version TEXT NOT NULL,

    match TEXT NOT NULL,

    UNIQUE(job_id, seq_id)
);

CREATE TABLE db (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,
    xxh64 INTEGER UNIQUE NOT NULL,
    filepath TEXT UNIQUE NOT NULL
);

COMMIT TRANSACTION;

PRAGMA foreign_keys = ON;
