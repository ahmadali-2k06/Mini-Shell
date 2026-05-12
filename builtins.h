#ifndef BUILTINS_H
#define BUILTINS_H

#include "parser.h"

int is_builtin(const char *cmd);
int execute_builtin(parsed_cmd_t *cmd);

// Built-in commands
int builtin_cd(parsed_cmd_t *cmd);
int builtin_exit(void);
void builtin_help(void);
void builtin_jobs(void);

// Stubs for Module 2 & 3 commands
int cmd_submit(parsed_cmd_t *cmd);
int cmd_schedule(parsed_cmd_t *cmd);
int cmd_gantt(void);
int cmd_race(void);
int cmd_sync_demo(void);

#endif // BUILTINS_H
