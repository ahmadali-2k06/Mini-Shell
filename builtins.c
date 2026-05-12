#include "builtins.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int is_builtin(const char *cmd) {
    if (!cmd) return 0;
    
    const char *builtins[] = {
        "cd", "exit", "help", "jobs",
        "submit", "schedule", "gantt",
        "race", "sync_demo", NULL
    };

    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(cmd, builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int execute_builtin(parsed_cmd_t *cmd) {
    if (!cmd || cmd->argc == 0) return -1;
    
    const char *name = cmd->args[0];

    if (strcmp(name, "cd") == 0) {
        return builtin_cd(cmd);
    } else if (strcmp(name, "exit") == 0) {
        return builtin_exit();
    } else if (strcmp(name, "help") == 0) {
        builtin_help();
        return 0;
    } else if (strcmp(name, "jobs") == 0) {
        builtin_jobs();
        return 0;
    } else if (strcmp(name, "submit") == 0) {
        return cmd_submit(cmd);
    } else if (strcmp(name, "schedule") == 0) {
        return cmd_schedule(cmd);
    } else if (strcmp(name, "gantt") == 0) {
        return cmd_gantt();
    } else if (strcmp(name, "race") == 0) {
        return cmd_race();
    } else if (strcmp(name, "sync_demo") == 0) {
        return cmd_sync_demo();
    }

    return -1;
}

int builtin_cd(parsed_cmd_t *cmd) {
    char *path = cmd->args[1];
    
    if (path == NULL) {
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "minishell: cd: HOME not set\n");
            return -1;
        }
    }

    if (chdir(path) != 0) {
        perror("minishell: cd");
        return -1;
    }
    return 0;
}

int builtin_exit(void) {
    cleanup_jobs();
    exit(0);
}

void builtin_help(void) {
    printf("Mini Shell - Available Built-in Commands:\n");
    printf("  cd <dir>            Change working directory\n");
    printf("  exit                Exit the shell\n");
    printf("  help                Print this help message\n");
    printf("  jobs                List all background and queued jobs\n");
    printf("  submit <cmd> <b_t>  Submit a job to the scheduler (Module 2)\n");
    printf("  schedule <algo> [q] Run the scheduler (Module 2)\n");
    printf("  gantt               Display Gantt chart of last schedule run (Module 2)\n");
    printf("  race                Run race condition demo (Module 3)\n");
    printf("  sync_demo           Run synchronized demo (Module 3)\n");
}

void builtin_jobs(void) {
    print_job_list();
}


