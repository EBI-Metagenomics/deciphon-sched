PRAGMA foreign_keys = off;

BEGIN TRANSACTION;

CREATE TABLE job (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,
    -- type: 0 for scan jobs; 1 for hmm jobs.
    type INTEGER CHECK(type IN (0, 1)) NOT NULL,

    state TEXT CHECK(state IN ('pend', 'run', 'done', 'fail')) NOT NULL,
    progress INTEGER CHECK(0 <= progress AND progress <= 100) NOT NULL,
    error TEXT NOT NULL,

    submission INTEGER NOT NULL,
    exec_started INTEGER NOT NULL,
    exec_ended INTEGER NOT NULL
);

CREATE TABLE hmm (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,
    xxh3 INTEGER UNIQUE NOT NULL,
    filename TEXT UNIQUE NOT NULL,

    job_id INTEGER REFERENCES job (id) NOT NULL
);

CREATE TABLE db (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,
    xxh3 INTEGER UNIQUE NOT NULL,
    filename TEXT UNIQUE NOT NULL,

    hmm_id INTEGER REFERENCES hmm (id) NOT NULL
);

CREATE TABLE scan (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,
    db_id INTEGER REFERENCES db (id) NOT NULL,

    multi_hits INTEGER NOT NULL,
    hmmer3_compat INTEGER NOT NULL,

    job_id INTEGER REFERENCES job (id) NOT NULL
);

CREATE TABLE seq (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,
    scan_id INTEGER REFERENCES scan (id) NOT NULL,
    name TEXT NOT NULL,
    data TEXT NOT NULL
);

CREATE TABLE prod (
    id INTEGER PRIMARY KEY UNIQUE NOT NULL,

    scan_id INTEGER REFERENCES scan (id) NOT NULL,
    seq_id INTEGER REFERENCES seq (id) NOT NULL,

    profile_name TEXT NOT NULL,
    abc_name TEXT NOT NULL,

    alt_loglik REAL NOT NULL,
    null_loglik REAL NOT NULL,

    profile_typeid TEXT NOT NULL,
    version TEXT NOT NULL,

    match TEXT NOT NULL,

    UNIQUE(scan_id, seq_id, profile_name)
);

COMMIT TRANSACTION;

PRAGMA foreign_keys = ON;
