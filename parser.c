#include "parser.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void trim_whitespace(char *str) {
    if (!str) return;

    // Trim leading space
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

int parse_input(char *line, parsed_cmd_t *cmd) {
    if (!line || !cmd) return -1;

    cmd->argc = 0;
    cmd->is_background = 0;
    for (int i = 0; i < 64; i++) {
        cmd->args[i] = NULL;
    }

    trim_whitespace(line);
    if (strlen(line) == 0) {
        return -1; // Empty line
    }

    // Simple quote handling and tokenization
    char *p = line;
    while (*p) {
        // Skip whitespace
        while (isspace((unsigned char)*p)) {
            *p = '\0';
            p++;
        }
        if (*p == '\0') break;

        // Check for background '&'
        if (*p == '&' && *(p + 1) == '\0') {
            cmd->is_background = 1;
            *p = '\0';
            break;
        }

        if (cmd->argc < 63) {
            if (*p == '"' || *p == '\'') {
                char quote = *p;
                p++; // Skip quote
                cmd->args[cmd->argc++] = p;
                while (*p && *p != quote) {
                    p++;
                }
                if (*p == quote) {
                    *p = '\0';
                    p++;
                }
            } else {
                cmd->args[cmd->argc++] = p;
                while (*p && !isspace((unsigned char)*p) && *p != '&') {
                    p++;
                }
                if (*p == '&' && (isspace((unsigned char)*(p+1)) || *(p+1) == '\0')) {
                    // Stop token here, '&' will be caught in next loop
                } else if (*p != '\0') {
                    *p = '\0';
                    p++;
                }
            }
        } else {
            // Reached max arguments
            while (*p && !isspace((unsigned char)*p)) p++;
        }
    }

    cmd->args[cmd->argc] = NULL;

    if (cmd->argc > 0 && strcmp(cmd->args[cmd->argc - 1], "&") == 0) {
        cmd->is_background = 1;
        cmd->args[--cmd->argc] = NULL;
    }

    if (cmd->argc == 0) {
        return -1;
    }

    return 0;
}
