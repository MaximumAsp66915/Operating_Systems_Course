// t_pinfo: checks the getpinfo system call and the birth field.
// Verifies: every active process is listed, pid/ppid/name/state are
// correct, and ages follow the process hierarchy (init is the oldest,
// then its child, then the current process).
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
failed(const char *msg)
{
  printf("t_pinfo: FAILED: %s\n", msg);
  exit(1);
}

static int
valid_state(const char *s)
{
  return strcmp(s, "used") == 0 || strcmp(s, "sleeping") == 0 ||
         strcmp(s, "runnable") == 0 || strcmp(s, "running") == 0 ||
         strcmp(s, "zombie") == 0;
}

int
main(int argc, char *argv[])
{
  struct pinfo info[64];
  struct pinfo self, initp, parentp;
  int me = getpid();
  int found_self = 0, found_init = 0, found_parent = 0;
  int n, i;

  n = getpinfo(info, 64);
  if (n < 0)
    failed("getpinfo returned -1");
  if (n < 3)
    failed("fewer than 3 processes listed");

  for (i = 0; i < n; i++) {
    if (info[i].pid == me) {
      self = info[i];
      found_self = 1;
    }
    if (info[i].pid == 1) {
      initp = info[i];
      found_init = 1;
    }
    if (info[i].pid == info[i].ppid && info[i].pid != 0)
      failed("a process has pid == ppid");
    if (info[i].pid == 0)
      failed("a process has pid == 0");
  }
  if (!found_self)
    failed("the calling process is not listed");
  if (!found_init)
    failed("init (pid 1) is not listed");

  for (i = 0; i < n; i++) {
    if (info[i].pid == self.ppid) {
      parentp = info[i];
      found_parent = 1;
    }
  }
  if (!found_parent)
    failed("the parent process is not listed");

  if (strcmp(self.state, "running") != 0)
    failed("the calling process should be in state running");
  if (strcmp(self.name, "t_pinfo") != 0)
    failed("wrong name for the calling process");
  if (initp.ppid != 0)
    failed("init must have ppid 0");
  if (strcmp(initp.name, "init") != 0)
    failed("wrong name for init");
  if (initp.age < parentp.age)
    failed("init is younger than its child (check the birth field)");
  // Strict: with a correct birth field the parent is always older
  // than the process that was just started. If the birth field is
  // never set (stays 0) every age equals the current tick count and
  // these checks fail.
  if (parentp.age <= self.age)
    failed("parent must be strictly older than the calling process");
  if (self.age > 20)
    failed("the calling process looks older than 20 ticks — "
           "was the birth field ever set?");

  for (i = 0; i < n; i++) {
    if (!valid_state(info[i].state))
      failed("bad state string (want used/sleeping/runnable/running/zombie)");
    if (info[i].name[0] == 0)
      failed("empty process name");
  }

  printf("t_pinfo: OK\n");
  exit(0);
}
