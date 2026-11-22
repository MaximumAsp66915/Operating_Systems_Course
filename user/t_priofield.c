// t_priofield: checks the priority field added to struct proc.
// Verifies that allocproc() initialises priority to 0 for every process:
// the caller itself, a forked child, and a child forked after the parent
// changed its own priority.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
failed(const char *msg)
{
  printf("t_priofield: FAILED: %s\n", msg);
  exit(1);
}

static void
wait_child(int pid, const char *what)
{
  int status;

  if (wait(&status) < 0)
    failed("wait failed");
  if (status != 0) {
    printf("t_priofield: FAILED: %s\n", what);
    exit(1);
  }
}

int
main(void)
{
  int me = getpid();
  int pid;

  // A freshly started process must have the default priority 0.
  if (getpriority(me) != 0)
    failed("a fresh process must start with priority 0");

  // A forked child must also start at 0 (allocproc sets it).
  pid = fork();
  if (pid < 0)
    failed("fork failed");
  if (pid == 0) {
    if (getpriority(getpid()) != 0)
      failed("a forked child must start with priority 0");
    exit(0);
  }
  wait_child(pid, "first forked child did not start with priority 0");

  // Change the parent's priority, then fork again: the new child must
  // still start at 0, not inherit the parent's value.
  if (setpriority(me, 77) != 0)
    failed("setpriority(self, 77) failed");
  if (getpriority(me) != 77)
    failed("setpriority(self, 77) did not take effect");

  pid = fork();
  if (pid < 0)
    failed("second fork failed");
  if (pid == 0) {
    if (getpriority(getpid()) != 0)
      failed("a child must not inherit the parent's priority");
    exit(0);
  }
  wait_child(pid, "second forked child did not start with priority 0");

  printf("t_priofield: OK\n");
  exit(0);
}
