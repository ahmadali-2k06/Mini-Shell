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

    job_entry_t *jobs_snapshot = malloc(sizeof(job_entry_t) * queue->count);
    int snap_count = 0;
    int current_time_offset = 0;
    time_t schedule_start = time(NULL);

    // Step 1: Fork all READY jobs into STOPPED state
    job_entry_t *curr = queue->head;
    while (curr) {
        if (curr->state == JOB_READY) {
            jobs_snapshot[snap_count] = *curr;

            pid_t pid = fork_and_exec_job(curr, 1);
            if (pid > 0) {
                int status;
                waitpid(pid, &status, WUNTRACED);
                curr->state = JOB_STOPPED;
                jobs_snapshot[snap_count].pid = pid;
            }
            snap_count++;
        }
        curr = curr->next;
    }

    // Step 2: Round Robin dispatch
    int jobs_remaining = snap_count;

    while (jobs_remaining > 0) {
        curr = queue->head;

        while (curr) {
            if (curr->state == JOB_STOPPED) {

                // Set start_time on first execution
                if (curr->start_time == 0) {
                    curr->start_time = schedule_start + current_time_offset;
                    for (int i = 0; i < snap_count; i++) {
                        if (jobs_snapshot[i].job_id == curr->job_id) {
                            jobs_snapshot[i].start_time = curr->start_time;
                            break;
                        }
                    }
                }

                kill(curr->pid, SIGCONT);
                curr->state = JOB_RUNNING;

                int gantt_start = current_time_offset;

                int actual_used = (curr->remaining_burst > quantum)
                                  ? quantum
                                  : curr->remaining_burst;

                sleep(actual_used);
                curr->remaining_burst -= actual_used;
                current_time_offset += actual_used;

                // ---- THE FIX ----
                // Check status carefully — WIFSTOPPED also triggers WUNTRACED
                // and returns the pid, so we must explicitly exclude it
                int status;
                pid_t res = waitpid(curr->pid, &status, WNOHANG | WUNTRACED);

                int process_died = (res == curr->pid) &&
                                   (WIFEXITED(status) || WIFSIGNALED(status));
                                   // NOT WIFSTOPPED — that means it's paused, not dead

                if (process_died || curr->remaining_burst <= 0) {
                    if (!process_died) {
                        kill(curr->pid, SIGTERM);
                        // Wait for it to actually die after SIGTERM
                        waitpid(curr->pid, &status, 0);
                    }

                    curr->finish_time = schedule_start + current_time_offset;
                    curr->state = JOB_FINISHED;
                    jobs_remaining--;

                    for (int i = 0; i < snap_count; i++) {
                        if (jobs_snapshot[i].job_id == curr->job_id) {
                            jobs_snapshot[i].finish_time = curr->finish_time;
                            jobs_snapshot[i].state = JOB_FINISHED;
                            break;
                        }
                    }
                } else {
                    // Still has burst left — pause it
                    kill(curr->pid, SIGSTOP);
                    waitpid(curr->pid, &status, WUNTRACED); // Wait until stopped
                    curr->state = JOB_STOPPED;
                }

                // Gantt entry
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

    for (int i = 0; i < snap_count; i++) {
        jobs_snapshot[i].arrival_time = schedule_start;
    }

    compute_stats(jobs_snapshot, snap_count);
    free(jobs_snapshot);
    return 0;
}