#include "sjf.h"
#include "scheduler.h"
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

job_entry_t *find_shortest_job(job_queue_t *queue) {
    if (!queue) return NULL;

    job_entry_t *shortest = NULL;
    job_entry_t *curr = queue->head;

    while (curr) {
        if (curr->state == JOB_READY && curr->pid == 0) {
            if (!shortest) {
                shortest = curr;
            } else if (curr->burst_time < shortest->burst_time) {
                shortest = curr;
            } else if (curr->burst_time == shortest->burst_time) {
                // FCFS tie breaker
                if (curr->arrival_time < shortest->arrival_time) {
                    shortest = curr;
                }
            }
        }
        curr = curr->next;
    }
    
    return shortest;
}

int schedule_sjf(job_queue_t *queue, schedule_result_t *result) {
    if (!queue || !result) return -1;

    // We will collect jobs for stats reporting
    job_entry_t *jobs_snapshot = malloc(sizeof(job_entry_t) * queue->count);
    int snap_count = 0;
    
    int current_time_offset = 0;
    int p_index = 1;

    // We need to map actual jobs to P1, P2 based on submission order to match Gantt labels
    // First, let's assign P_indexes sequentially by scanning queue
    // (This is a simplified approach; realistically we just assign P_index as they were submitted)
    // Actually, P_index should be based on `job_id`. Let's just use `job_id` directly for P_index
    // But the spec says "P1, P2", so we can format "P%d", job->job_id.

    while (1) {
        job_entry_t *curr = find_shortest_job(queue);
        if (!curr) break; // No more ready jobs
        
        jobs_snapshot[snap_count] = *curr;
        
        // Dispatch
        update_job_state(0, JOB_RUNNING);
        curr->state = JOB_RUNNING;
        curr->start_time = time(NULL);
        
        pid_t pid = fork_and_exec_job(curr, 0);
        
        if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            
            curr->finish_time = time(NULL);
            curr->state = JOB_FINISHED;
            
            if (result->entry_count < 256) {
                gantt_entry_t *ge = &result->entries[result->entry_count++];
                ge->job_id = curr->job_id;
                sprintf(ge->label, "P%d", curr->job_id); // Label by Job ID
                ge->start = current_time_offset;
                
                int elapsed = (int)(curr->finish_time - curr->start_time);
                if (elapsed == 0) elapsed = 1;
                current_time_offset += elapsed;
                ge->end = current_time_offset;
            }
            
            jobs_snapshot[snap_count].start_time = curr->start_time;
            jobs_snapshot[snap_count].finish_time = curr->finish_time;
            jobs_snapshot[snap_count].state = JOB_FINISHED;
        }
        
        snap_count++;
    }

    result->total_time = current_time_offset;
    compute_stats(jobs_snapshot, snap_count);
    
    free(jobs_snapshot);
    return 0;
}
