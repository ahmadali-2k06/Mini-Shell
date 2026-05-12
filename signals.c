#include "signals.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

void sigint_handler(int sig) {
    (void)sig;
    // Just print a newline and a new prompt. We don't exit the shell.
    // write() is async-signal-safe, unlike printf()
    write(STDOUT_FILENO, "\nminishell$ ", 12);
}

void sigtstp_handler(int sig) {
    (void)sig;
    // Ignore Ctrl+Z in the shell process itself.
    // If there's a foreground process, the terminal will send SIGTSTP to the foreground process group.
}

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    
    // Reap all dead child processes
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        job_entry_t *job = find_job_by_pid(pid);
        
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                update_job_state(pid, JOB_FINISHED);
            } else if (WIFSTOPPED(status)) {
                update_job_state(pid, JOB_STOPPED);
            } else if (WIFCONTINUED(status)) {
                update_job_state(pid, JOB_RUNNING);
            }
        }
    }
}

void install_signal_handlers(void) {
    struct sigaction sa_int;
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("minishell: sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_tstp;
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("minishell: sigaction SIGTSTP");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP; // We might want to see stopped jobs for RR
    sa_chld.sa_flags = SA_RESTART; // Need to see stops/conts
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("minishell: sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
}
