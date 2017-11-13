#include <stdio.h>
#include <unistd.h>

int main() {

  int pid = fork();

  if (pid == 0) {
    execv("./a.out", NULL);
  } else {
    printf("wait\n");
    int status;
    wait(&status);
    printf("end\n");
  }

  return 0;
}
