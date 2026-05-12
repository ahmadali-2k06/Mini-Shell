#include "rr.h"
#include "scheduler.h"
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

int schedule_rr(job_queue_t *queue, int quantum, schedule_result_t *result) {
    if (!queue || !result || quantum < 1) return -1;

    // Snapshot jobs for stats
    job_entry_t *jobs_snapshot = malloc(sizeof(job_entry_t) * queue->count);
    int snap_count = 0;

    int current_time_offset = 0;

    // Step 1: Fork all READY jobs into STOPPED state
    job_entry_t *curr = queue->head;
    while (curr) {
        if (curr->state == JOB_READY && curr->pid == 0) {
            jobs_snapshot[snap_count] = *curr;
            
            // start_stopped = 1
            pid_t pid = fork_and_exec_job(curr, 1);
            if (pid > 0) {
                // Wait for it to actually stop before proceeding
                int status;
                waitpid(pid, &status, WUNTRACED);
                
                curr->state = JOB_STOPPED;
                jobs_snapshot[snap_count].pid = pid;
            }
            snap_count++;
        }
        curr = curr->next;
    }

    // Step 2: Cycle through jobs
    int all_finished = 0;
    while (!all_finished) {
        all_finished = 1;
        curr = queue->head;
        
        while (curr) {
            if (curr->state == JOB_STOPPED || curr->state == JOB_READY) {
                // We have a job to run
                all_finished = 0;
                
                if (curr->start_time == 0) {
                    curr->start_time = time(NULL);
                    
                    // Update snapshot with start time
                    for(int i=0; i<snap_count; i++) {
                        if (jobs_snapshot[i].job_id == curr->job_id) {
                            jobs_snapshot[i].start_time = curr->start_time;
                            break;
                        }
                    }
                }
                
                // Resume job
                kill(curr->pid, SIGCONT);
                curr->state = JOB_RUNNING;
                update_job_state(curr->pid, JOB_RUNNING);
                
                // Record Gantt start
                int gantt_start = current_time_offset;
                
                // Wait for quantum
                sleep(quantum);
                
                // Check if it finished during quantum
                int status;
                pid_t res = waitpid(curr->pid, &status, WNOHANG | WUNTRACED);
                
                if (res == curr->pid && (WIFEXITED(status) || WIFSIGNALED(status))) {
                    // It finished
                    curr->finish_time = time(NULL);
                    curr->state = JOB_FINISHED;
                    update_job_state(curr->pid, JOB_FINISHED);
                    
                    for(int i=0; i<snap_count; i++) {
                        if (jobs_snapshot[i].job_id == curr->job_id) {
                            jobs_snapshot[i].finish_time = curr->finish_time;
                            jobs_snapshot[i].state = JOB_FINISHED;
                            break;
                        }
                    }
                    
                    // The job finished within quantum
                    // Realistically, elapsed time is less than quantum, but we'll approximate with quantum for Gantt
                    current_time_offset += quantum; 
                } else {
                    // It's still running, pause it
                    kill(curr->pid, SIGSTOP);
                    
                    // Wait for it to stop
                    waitpid(curr->pid, &status, WUNTRACED);
                    
                    curr->state = JOB_STOPPED;
                    update_job_state(curr->pid, JOB_STOPPED);
                    
                    current_time_offset += quantum;
                }
                
                // Record Gantt end
                if (result->entry_count < 256) {
                    gantt_entry_t *ge = &result->entries[result->entry_count++];
                    ge->job_id = curr->job_id;
                    sprintf(ge->label, "P%d", curr->job_id);
                    ge->start = gantt_start;
                    ge->end = current_time_offset;
                }
            }
            curr = curr->next;
        }
    }

    result->total_time = current_time_offset;
    compute_stats(jobs_snapshot, snap_count);
    
    free(jobs_snapshot);
    return 0;
}
