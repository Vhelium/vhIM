#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

int process_command(char *input_buffer, int (*exec_cmd)(int, char**));

#endif

