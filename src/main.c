#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "signal_handlers.h"
#include "commands.h"
#include "built_in.h"
#include "utils.h"

int main()
{
  char buf[8096];

  char* paths = getenv("PATH");
  char* path = strtok(paths, ":");

  while(path != NULL) {
    g_paths[g_path_count++] = path;
    path = strtok(NULL, ":");
  }
  g_paths[g_path_count] = NULL;


  while (1) {
    if (signal(SIGINT, catch_sigint) == SIG_ERR) {
      printf("\nCAN NOT CATCH SIGINT");
    }

    if (signal(SIGTSTP, catch_sigtstp) == SIG_ERR) {
      printf("\nCAN NOT CATCH SIGINT");
    }
    fgets(buf, 8096, stdin);

    struct single_command commands[512];
    int n_commands = 0;
    mysh_parse_command(buf, &n_commands, &commands);

    int ret = evaluate_command(n_commands, &commands);

    free_commands(n_commands, &commands);
    n_commands = 0;

    if (ret == 1) {
      break;
    }
  }

  return 0;
}
