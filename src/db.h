#ifndef DB_H
#define DB_H

#include "sched/rc.h"

enum sched_rc db_delete(void);
void db_to_hmm_filename(char *filename);

#endif
