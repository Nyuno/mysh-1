#ifndef MYSH_COMMANDS_H_
#define MYSH_COMMANDS_H_

extern int g_path_count;
extern char* g_paths[256];

struct single_command
{
  int argc;
  char** argv;
};

int execute_command(char* command, char** argv, int argc);

int evaluate_command(int n_commands, struct single_command (*commands)[512]);

void free_commands(int n_commands, struct single_command (*commands)[512]);

#endif // MYSH_COMMANDS_H_
