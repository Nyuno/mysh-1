#include <stdio.h>
#include <signal.h>

#include "signal_handlers.h"

void catch_sigint(int signalNo)
{
  if (signalNo == SIGINT) {
    // NONE
  }
}

void catch_sigtstp(int signalNo)
{
  if (signalNo == SIGTSTP) {
    // NONE
  }
}
