#ifndef SCAN_H
#define SCAN_H

#include <stdint.h>

enum sched_rc scan_submit(void *scan, int64_t job_id);
enum sched_rc scan_delete(void);

#endif
