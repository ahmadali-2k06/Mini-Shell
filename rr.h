#ifndef RR_H
#define RR_H

#include "jobs.h"
#include "gantt.h"

int schedule_rr(job_queue_t *queue, int quantum, schedule_result_t *result);

#endif // RR_H
