#ifndef SJF_H
#define SJF_H

#include "jobs.h"
#include "gantt.h"

int schedule_sjf(job_queue_t *queue, schedule_result_t *result);
job_entry_t *find_shortest_job(job_queue_t *queue);

#endif // SJF_H
