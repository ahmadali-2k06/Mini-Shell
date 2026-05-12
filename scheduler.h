#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "parser.h"
#include "jobs.h"
#include "gantt.h"

// Expose these for the scheduler commands in builtins.c
int cmd_submit(parsed_cmd_t *cmd);
int cmd_schedule(parsed_cmd_t *cmd);
int cmd_gantt(void);

// Shared helper for all algorithms
pid_t fork_and_exec_job(job_entry_t *job, int start_stopped);

// Stats
void compute_stats(job_entry_t *jobs_snapshot, int count);

// External global declarations for state persistence
extern schedule_result_t last_schedule;
extern job_stats_t last_stats[64];
extern int last_stats_count;

#endif // SCHEDULER_H
