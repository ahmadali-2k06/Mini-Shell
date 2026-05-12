#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "jobs.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void shell_loop(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    parsed_cmd_t cmd;
    char cwd[1024];

    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("\n%s$ ", cwd);
        } else {
            printf("\nminishell$ ");
        }
        
        fflush(stdout);

        read = getline(&line, &len, stdin);
        
        if (read == -1) {
            // EOF (Ctrl+D)
            printf("\n");
            break;
        }

        if (parse_input(line, &cmd) == 0) {
            if (is_builtin(cmd.args[0])) {
                execute_builtin(&cmd);
            } else {
                execute_external(&cmd);
            }
        }
    }

    free(line);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    init_job_queue();
    install_signal_handlers();

    shell_loop();

    cleanup_jobs();
    return 0;
}
