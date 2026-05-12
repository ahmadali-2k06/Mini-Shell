#include "fcfs.h"
#include "scheduler.h"
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

int schedule_fcfs(job_queue_t *queue, schedule_result_t *result) {
    if (!queue || !result) return -1;

    job_entry_t *jobs_snapshot = malloc(sizeof(job_entry_t) * queue->count);
    int snap_count = 0;
    
    // Use a fixed start time for stats calculation
    time_t schedule_start = time(NULL);
    int current_time_offset = 0;

    job_entry_t *curr = queue->head;
    while (curr) {
        if (curr->state == JOB_READY && curr->pid == 0) {
            
            jobs_snapshot[snap_count] = *curr;
            
            // Dispatch
            curr->state = JOB_RUNNING;
            // Set start_time based on current simulation offset
            curr->start_time = schedule_start + current_time_offset;
            
            pid_t pid = fork_and_exec_job(curr, 0); 
            
            if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                
                // IMPORTANT: Use simulated burst_time for the Gantt Chart
                int elapsed = curr->burst_time; 
                
                if (result->entry_count < 256) {
                    gantt_entry_t *ge = &result->entries[result->entry_count++];
                    ge->job_id = curr->job_id;
                    sprintf(ge->label, "P%d", curr->job_id); // Use Job ID for consistency
                    ge->start = current_time_offset;
                    
                    current_time_offset += elapsed;
                    ge->end = current_time_offset;
                }

                curr->finish_time = schedule_start + current_time_offset;
                curr->state = JOB_FINISHED;
                
                // Update snapshot for stats
                jobs_snapshot[snap_count].start_time = curr->start_time;
                jobs_snapshot[snap_count].finish_time = curr->finish_time;
                // Normalize arrival time to the start of scheduling
                jobs_snapshot[snap_count].arrival_time = schedule_start;
                jobs_snapshot[snap_count].state = JOB_FINISHED;
            }
            
            snap_count++;
        }
        curr = curr->next;
    }

    result->total_time = current_time_offset;
    compute_stats(jobs_snapshot, snap_count);
    
    free(jobs_snapshot);
    return 0;
}