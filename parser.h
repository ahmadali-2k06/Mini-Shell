#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char  *args[64];        // Tokenized arguments (args[0] = command name)
    int    argc;            // Number of tokens
    int    is_background;   // 1 if trailing '&' detected, 0 otherwise
} parsed_cmd_t;

int parse_input(char *line, parsed_cmd_t *cmd);
void trim_whitespace(char *str);

#endif // PARSER_H
