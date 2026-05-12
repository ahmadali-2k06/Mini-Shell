#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <time.h>

typedef enum {
    JOB_READY,      // Queued, not yet started
    JOB_RUNNING,    // Currently executing
    JOB_STOPPED,    // Paused via SIGSTOP (used by RR scheduler)
    JOB_FINISHED    // Completed execution
} job_state_t;

typedef struct job_entry {
    int            job_id;        // Shell-assigned sequential ID (for `jobs` display)
    pid_t          pid;           // PID after fork(); 0 before fork
    char           command[256];  // Raw command string as entered
    int            burst_time;    // User-estimated seconds (for SJF); -1 if unknown
    time_t         arrival_time;  // time() when job was submitted
    time_t         start_time;    // time() when job first got CPU; 0 if not started
    time_t         finish_time;   // time() when job completed; 0 if not finished
    job_state_t    state;         // Current state
    struct job_entry *next;       // Linked list pointer
} job_entry_t;

typedef struct {
    job_entry_t *head;
    job_entry_t *tail;
    int          count;
    int          next_job_id;    // Auto-increment counter
} job_queue_t;

void init_job_queue(void);
job_entry_t *add_job(pid_t pid, const char *command, int burst_time);
void remove_job(pid_t pid);
job_entry_t *find_job_by_pid(pid_t pid);
void print_job_list(void);
void update_job_state(pid_t pid, job_state_t new_state);
void cleanup_jobs(void);
job_queue_t *get_job_queue(void);

#endif // JOBS_H
