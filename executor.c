#include "executor.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

int execute_external(parsed_cmd_t *cmd) {
    if (cmd->argc == 0 || !cmd->args[0]) return -1;

    pid_t pid = fork();

    if (pid < 0) {
        perror("minishell: fork failed");
        return -1;
    } else if (pid == 0) {
        // Child process
        
        // Put the child in its own process group
        setpgid(0, 0);

        if (execvp(cmd->args[0], cmd->args) == -1) {
            fprintf(stderr, "minishell: %s: command not found\n", cmd->args[0]);
            _exit(127);
        }
    } else {
        // Parent process
        
        // Construct the full command string for display
        char full_cmd[256] = "";
        for (int i = 0; i < cmd->argc; i++) {
            strncat(full_cmd, cmd->args[i], sizeof(full_cmd) - strlen(full_cmd) - 1);
            if (i < cmd->argc - 1) {
                strncat(full_cmd, " ", sizeof(full_cmd) - strlen(full_cmd) - 1);
            }
        }

        if (cmd->is_background) {
            // Register as a background job
            job_entry_t *new_job = add_job(pid, full_cmd, -1);
            if (new_job) {
                new_job->state = JOB_RUNNING;
                new_job->start_time = time(NULL);
                printf("[%d] %d\n", new_job->job_id, pid);
            }
        } else {
            // Foreground job
            // Need to set the terminal foreground process group to the child, but we aren't doing full job control yet.
            // Let's just wait for it.
            int status;
            while (waitpid(pid, &status, 0) == -1) {
                if (errno != EINTR) {
                    break;
                }
            }
        }
    }

    return 0;
}
