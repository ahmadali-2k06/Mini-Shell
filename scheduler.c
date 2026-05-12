#include "scheduler.h"
#include "fcfs.h"
#include "sjf.h"
#include "rr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

schedule_result_t last_schedule;
job_stats_t last_stats[64];
int last_stats_count = 0;
static int schedule_has_run = 0;

int cmd_submit(parsed_cmd_t *cmd) {
    if (cmd->argc < 3) {
        fprintf(stderr, "Usage: submit <command> <burst_time>\n");
        return -1;
    }

    // Command is from args[1] to args[argc-2]
    // Burst time is args[argc-1]
    
    int burst_time = atoi(cmd->args[cmd->argc - 1]);
    if (burst_time <= 0) {
        fprintf(stderr, "minishell: Burst time must be a positive integer\n");
        return -1;
    }

    char full_cmd[256] = "";
    for (int i = 1; i < cmd->argc - 1; i++) {
        strncat(full_cmd, cmd->args[i], sizeof(full_cmd) - strlen(full_cmd) - 1);
        if (i < cmd->argc - 2) {
            strncat(full_cmd, " ", sizeof(full_cmd) - strlen(full_cmd) - 1);
        }
    }

    // We add job with PID 0 because it's not forked yet
    job_entry_t *new_job = add_job(0, full_cmd, burst_time);
    if (new_job) {
        printf("Job [%d] submitted: %s (Burst: %ds)\n", new_job->job_id, full_cmd, burst_time);
    }
    
    return 0;
}

void compute_stats(job_entry_t *jobs_snapshot, int count) {
    last_stats_count = 0;
    
    for (int i = 0; i < count && i < 64; i++) {
        job_entry_t *job = &jobs_snapshot[i];
        
        last_stats[i].job_id = job->job_id;
        strncpy(last_stats[i].command, job->command, sizeof(last_stats[i].command) - 1);
        last_stats[i].command[sizeof(last_stats[i].command) - 1] = '\0';
        
        last_stats[i].turnaround_time = (int)(job->finish_time - job->arrival_time);
        last_stats[i].response_time = (int)(job->start_time - job->arrival_time);
        last_stats[i].waiting_time = last_stats[i].turnaround_time - job->burst_time;
        
        if (last_stats[i].waiting_time < 0) last_stats[i].waiting_time = 0;
        
        last_stats_count++;
    }
}

int cmd_schedule(parsed_cmd_t *cmd) {
    if (cmd->argc < 2) {
        fprintf(stderr, "Usage: schedule <fcfs|sjf|rr> [quantum]\n");
        return -1;
    }

    job_queue_t *queue = get_job_queue();
    
    // Check if queue has READY jobs
    int ready_count = 0;
    job_entry_t *curr = queue->head;
    while (curr) {
        if (curr->state == JOB_READY && curr->pid == 0) {
            ready_count++;
        }
        curr = curr->next;
    }

    if (ready_count == 0) {
        printf("No jobs in queue to schedule.\n");
        return 0;
    }

    // Temporarily block SIGCHLD so the shell's waitpid doesn't steal our statuses
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    last_schedule.entry_count = 0;
    last_schedule.total_time = 0;

    char *algo = cmd->args[1];
    int ret = 0;

    if (strcmp(algo, "fcfs") == 0) {
        ret = schedule_fcfs(queue, &last_schedule);
    } else if (strcmp(algo, "sjf") == 0) {
        ret = schedule_sjf(queue, &last_schedule);
    } else if (strcmp(algo, "rr") == 0) {
        int quantum = 2; // Default
        if (cmd->argc >= 3) {
            quantum = atoi(cmd->args[2]);
            if (quantum < 1) {
                fprintf(stderr, "minishell: Quantum must be >= 1 second\n");
                sigprocmask(SIG_SETMASK, &oldmask, NULL);
                return -1;
            }
        }
        ret = schedule_rr(queue, quantum, &last_schedule);
    } else {
        fprintf(stderr, "minishell: Unknown scheduling algorithm '%s'\n", algo);
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        return -1;
    }

    if (ret == 0) {
        schedule_has_run = 1;
        render_gantt(&last_schedule);
        print_stats(last_stats, last_stats_count);
        
        // Remove finished jobs from queue conceptually (or keep them but their state is FINISHED)
        // For simplicity, we just leave them as FINISHED and they won't be re-scheduled.
    }

    // Unblock SIGCHLD
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    return ret;
}

int cmd_gantt(void) {
    if (!schedule_has_run) {
        printf("No schedule has been run yet.\n");
        return 0;
    }
    render_gantt(&last_schedule);
    print_stats(last_stats, last_stats_count);
    return 0;
}

pid_t fork_and_exec_job(job_entry_t *job, int start_stopped) {
    // Parse the command string again
    parsed_cmd_t cmd;
    char cmd_copy[256];
    strncpy(cmd_copy, job->command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    if (parse_input(cmd_copy, &cmd) != 0) {
        fprintf(stderr, "minishell: failed to parse job command '%s'\n", job->command);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("minishell: schedule fork failed");
        return -1;
    } else if (pid == 0) {
        // Child
        setpgid(0, 0); // Own process group
        
        if (start_stopped) {
            raise(SIGSTOP);
        }

        if (execvp(cmd.args[0], cmd.args) == -1) {
            fprintf(stderr, "minishell: %s: command not found\n", cmd.args[0]);
            _exit(127);
        }
    }
    
    // Parent
    job->pid = pid;
    return pid;
}
