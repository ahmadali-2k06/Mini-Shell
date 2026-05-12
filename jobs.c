#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static job_queue_t global_job_queue;

void init_job_queue(void) {
    global_job_queue.head = NULL;
    global_job_queue.tail = NULL;
    global_job_queue.count = 0;
    global_job_queue.next_job_id = 1;
}

job_queue_t *get_job_queue(void) {
    return &global_job_queue;
}

job_entry_t *add_job(pid_t pid, const char *command, int burst_time) {
    job_entry_t *new_job = malloc(sizeof(job_entry_t));
    if (!new_job) {
        perror("minishell: malloc failed");
        return NULL;
    }

    new_job->job_id = global_job_queue.next_job_id++;
    new_job->pid = pid;
    strncpy(new_job->command, command, sizeof(new_job->command) - 1);
    new_job->command[sizeof(new_job->command) - 1] = '\0';
    new_job->burst_time = burst_time;
    new_job->remaining_burst = burst_time; //Copy to remaining_burst at start
    new_job->arrival_time = time(NULL);
    new_job->start_time = 0;
    new_job->finish_time = 0;
    new_job->state = JOB_READY;
    new_job->next = NULL;

    if (global_job_queue.tail) {
        global_job_queue.tail->next = new_job;
    } else {
        global_job_queue.head = new_job;
    }
    global_job_queue.tail = new_job;
    global_job_queue.count++;

    return new_job;
}

void remove_job(pid_t pid) {
    job_entry_t *curr = global_job_queue.head;
    job_entry_t *prev = NULL;

    while (curr) {
        if (curr->pid == pid) {
            if (prev) {
                prev->next = curr->next;
            } else {
                global_job_queue.head = curr->next;
            }
            if (curr == global_job_queue.tail) {
                global_job_queue.tail = prev;
            }
            free(curr);
            global_job_queue.count--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

job_entry_t *find_job_by_pid(pid_t pid) {
    job_entry_t *curr = global_job_queue.head;
    while (curr) {
        if (curr->pid == pid) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void print_job_list(void) {
    job_entry_t *curr = global_job_queue.head;
    if (!curr) {
        printf("No jobs.\n");
        return;
    }

    const char *state_strs[] = {"READY", "RUNNING", "STOPPED", "FINISHED"};

    while (curr) {
        printf("[%d] %s %s\n", curr->job_id, state_strs[curr->state], curr->command);
        curr = curr->next;
    }
}

void update_job_state(pid_t pid, job_state_t new_state) {
    job_entry_t *job = find_job_by_pid(pid);
    if (job) {
        job->state = new_state;
        if (new_state == JOB_RUNNING && job->start_time == 0) {
            job->start_time = time(NULL);
        } else if (new_state == JOB_FINISHED && job->finish_time == 0) {
            job->finish_time = time(NULL);
        }
    }
}

void cleanup_jobs(void) {
    job_entry_t *curr = global_job_queue.head;
    while (curr) {
        job_entry_t *next = curr->next;
        if (curr->pid > 0 && curr->state != JOB_FINISHED) {
            kill(curr->pid, SIGTERM);
            kill(curr->pid, SIGKILL);
        }
        free(curr);
        curr = next;
    }
    init_job_queue();
}
