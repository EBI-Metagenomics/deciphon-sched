#ifndef SCAN_H
#define SCAN_H

#include "sched/scan.h"
#include <stdbool.h>
#include <stdint.h>

enum sched_rc scan_submit(void *scan, int64_t job_id);
void scan_rollback(void);
enum sched_rc scan_delete(void);

#endif
