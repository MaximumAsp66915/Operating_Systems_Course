// t_priority: checks the setpriority/getpriority system calls.
// Verifies: set/get round-trip, clamping to [0,100], -1 for a pid that
// does not exist, and changing the priority of another process.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define BADPID 12345

static void
failed(const char *msg)
{
  printf("t_priority: FAILED: %s\n", msg);
  exit(1);
}

int
main(void)
{
  int me = getpid();
  int fds[2];
  int pid, status;
  char c = 'x';

  // Basic round-trip on the calling process.
  if (setpriority(me, 42) != 0)
    failed("setpriority(self, 42) must return 0");
  if (getpriority(me) != 42)
    failed("getpriority must return the value just set");

  // Values below the valid range are clamped to 0.
  if (setpriority(me, -10) != 0)
    failed("setpriority with a negative value must succeed (clamped)");
  if (getpriority(me) != 0)
    failed("priority -10 must be clamped to 0");

  // Values above the valid range are clamped to 100.
  if (setpriority(me, 250) != 0)
    failed("setpriority with a value above 100 must succeed (clamped)");
  if (getpriority(me) != 100)
    failed("priority 250 must be clamped to 100");

  // Boundary values must be accepted unchanged.
  if (setpriority(me, 0) != 0 || getpriority(me) != 0)
    failed("priority 0 must be accepted");
  if (setpriority(me, 100) != 0 || getpriority(me) != 100)
    failed("priority 100 must be accepted");

  // A pid that does not exist must yield -1 from both calls.
  if (setpriority(BADPID, 50) != -1)
    failed("setpriority with a nonexistent pid must return -1");
  if (getpriority(BADPID) != -1)
    failed("getpriority with a nonexistent pid must return -1");

  // A process must be able to change another process's priority.
  if (pipe(fds) < 0)
    failed("pipe failed");
  pid = fork();
  if (pid < 0)
    failed("fork failed");
  if (pid == 0) {
    close(fds[1]);
    // Block until the parent has changed our priority.
    if (read(fds[0], &c, 1) != 1)
      failed("child read failed");
    if (getpriority(getpid()) != 88)
      failed("child did not observe the priority set by its parent");
    close(fds[0]);
    exit(0);
  }
  close(fds[0]);
  if (setpriority(pid, 88) != 0)
    failed("setpriority(child, 88) must return 0");
  if (getpriority(pid) != 88)
    failed("getpriority(child) must return the new priority");
  if (write(fds[1], &c, 1) != 1)
    failed("parent write failed");
  close(fds[1]);

  if (wait(&status) < 0)
    failed("wait failed");
  if (status != 0)
    failed("child reported an error (see message above)");

  printf("t_priority: OK\n");
  exit(0);
}
