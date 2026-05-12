#include "fcfs.h"
#include "scheduler.h"
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

int schedule_fcfs(job_queue_t *queue, schedule_result_t *result) {
    if (!queue || !result) return -1;

    // We will collect jobs for stats reporting
    job_entry_t *jobs_snapshot = malloc(sizeof(job_entry_t) * queue->count);
    int snap_count = 0;
    
    // Base time for gantt chart is 0, which corresponds to the start of this schedule run
    time_t base_time = time(NULL);
    int current_time_offset = 0;

    int p_index = 1; // P1, P2, etc.

    job_entry_t *curr = queue->head;
    while (curr) {
        if (curr->state == JOB_READY && curr->pid == 0) {
            
            // It's a job to run. Record snapshot entry.
            jobs_snapshot[snap_count] = *curr;
            
            // Dispatch
            update_job_state(0, JOB_RUNNING); // Doesn't do much without pid, so set manually
            curr->state = JOB_RUNNING;
            curr->start_time = time(NULL);
            
            pid_t pid = fork_and_exec_job(curr, 0); // start immediately
            
            if (pid > 0) {
                // Wait for it to finish
                int status;
                waitpid(pid, &status, 0);
                
                curr->finish_time = time(NULL);
                curr->state = JOB_FINISHED;
                
                // Record gantt entry
                if (result->entry_count < 256) {
                    gantt_entry_t *ge = &result->entries[result->entry_count++];
                    ge->job_id = curr->job_id;
                    sprintf(ge->label, "P%d", p_index);
                    ge->start = current_time_offset;
                    
                    // FCFS is non-preemptive, so time elapsed is just burst_time or actual time
                    // Realistically, we use actual elapsed time
                    int elapsed = (int)(curr->finish_time - curr->start_time);
                    if (elapsed == 0) elapsed = 1; // Minimum 1 sec for display purposes
                    current_time_offset += elapsed;
                    ge->end = current_time_offset;
                }
                
                // Update snapshot with actual times
                jobs_snapshot[snap_count].start_time = curr->start_time;
                jobs_snapshot[snap_count].finish_time = curr->finish_time;
                jobs_snapshot[snap_count].state = JOB_FINISHED;
            }
            
            p_index++;
            snap_count++;
        }
        curr = curr->next;
    }

    result->total_time = current_time_offset;
    compute_stats(jobs_snapshot, snap_count);
    
    free(jobs_snapshot);
    return 0;
}
