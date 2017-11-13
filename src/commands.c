#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "commands.h"
#include "built_in.h"

int g_path_count          = 0;
int background_parent_pid = 0;
int background_pid        = 0;
int background_argc       = 0;
char* background_argv[256];
char* g_paths[256];

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

int execute_command(char* command, char** argv) {
  char command_path[256] = "";

  int pid = fork();

  if (pid == -1) {
    // Fail DO
    fprintf(stderr, "%s: unknown error; fork failed\n", command);

    return -1;
  } else if (pid == 0) {
    // Child DO
    execv(command, argv);

    for (int i = 0; i < g_path_count; ++i) {
      sprintf(command_path, "%s/%s", g_paths[i], command);

      execv(command_path, argv);
    }

    fprintf(stderr, "%s: command not found\n", command);

    return -1;
  } else {
    // Parent DO
    int status;
    wait(&status);
  }

  return 0;
}

int execute_command_without_fork(char* command, char** argv) {
}

int execute_command_with_socket(int input_stream, int output_stream, char* command, char** argv) {
  char command_path[256] = "";
  int pid = fork();

  if (pid == -1) {
    // Fail DO
    fprintf(stderr, "%s: unknown error; fork failed\n", command);

    return -1;
  } else if (pid == 0) {
    // Child DO
    if (input_stream != STDIN_FILENO) {
      dup2(input_stream, STDIN_FILENO);
      close(input_stream);
    }

    if (output_stream != STDOUT_FILENO) {
      dup2(output_stream, STDOUT_FILENO);
      close(output_stream);
    }

    execv(command, argv);

    for (int i = 0; i < g_path_count; ++i) {
      sprintf(command_path, "%s/%s", g_paths[i], command);

      execv(command_path, argv);
    }

    fprintf(stderr, "%s: command not found\n", command);
    exit(1);

    return 1;
  } else {
    // Parent DO
    int status;
    wait(&status);
  }

  return 0;
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);
    struct single_command* last_com = com + n_commands - 1;
    int saved_stdin = dup(0);
    int saved_stdout = dup(1);

    assert(com->argc != 0);

    // Background processing
    if (strcmp(last_com->argv[last_com->argc - 1], "&") == 0) {
      last_com->argv[last_com->argc - 1] = NULL;
      last_com->argc--;

      memcpy(background_argv, last_com->argv, sizeof(background_argv));
      background_argc = last_com->argc;

      background_parent_pid = fork();

      if (background_parent_pid == -1) {
        return 0;
      } else if (background_parent_pid == 0) {
        background_pid = fork();

        if (background_pid == -1) {
          exit(1);
        } else if (background_pid == 0) {
          //CHILD
          evaluate_command(n_commands, commands);

          return 1;
        } else {
          //PARENT
          int status;

          printf("%d\n", background_pid);

          if(wait(&status) == -1) {
            printf("%d fail ", background_pid);
          } else {
            printf("%d done ", background_pid);
          }

          for (int i = 0; i < background_argc; ++i) {
            printf("%s ", background_argv[i]);
          }
          printf("\n");

          return 1;
        }
      } else {
        return 0;
      }

      /*
      printf("pid running\t");
      for (int i = 0; i < background_argc; ++i) {
        printf("%s ", background_argv[i]);
      }
      printf("\n");
      */

      /*
       * TODO
       *  1. 프로세스 생성
       *  2. 자식 프로세스에서 Background용 프로세스 생성
       *  3. 자식 프로세스에서 Background용 프로세스 pid 출력
       *  4. 자식 프로세스에서 wait(Background용 프로세스)
       *  5. Background용 프로세스 끝나면 pid 출력과 함께 자식 프로세스 종료
       */
    }

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else {
      int pid = fork();
      int input_stream, sockfg[2];
      struct single_command* com  = (*commands);

      if (pid == -1) {
        fprintf(stderr, "%s: Fork fail\n", com->argv[0]);
        return -1;
      } else if (pid == 0) {
        close(saved_stdin);
        close(saved_stdout);

        input_stream = STDIN_FILENO;

        for (int i = 0; i < n_commands - 1; ++i) {
          socketpair(AF_UNIX, SOCK_STREAM, 0, sockfg);
          execute_command_with_socket(input_stream, sockfg[1], (com + i)->argv[0], (com + i)->argv);
          close(sockfg[1]);

          input_stream = sockfg[0];
        }

        if (input_stream != STDIN_FILENO) {
          dup2(input_stream, STDIN_FILENO);
          close(input_stream);
        }

        execute_command((com + n_commands - 1)->argv[0], (com + n_commands - 1)->argv);

        return 1;
      } else {
        int status;
        wait(&status);

        dup2(saved_stdin, 0);
        close(saved_stdin);
        dup2(saved_stdout, 1);
        close(saved_stdout);
      }

      return 0;
    }
  }

  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
