// t_sched: checks the priority scheduler.
//
// Phase 1: three children with priorities 90, 60 and 20 do the same amount
// of work. A correct scheduler runs the highest-priority child to
// completion first, so they must finish in strictly descending priority
// order. Each child writes its index to a pipe right before exiting, so
// the parent observes the real completion order (xv6's wait() returns
// zombies in process-table order, not in completion order).
//
// Phase 2: two children with the same priority (50) must both finish
// (round-robin among equal priorities must not starve either one).
//
// Children block on a pipe until the parent has set their priority, so
// they cannot start running with the default priority by mistake. The
// parent raises its own priority to 100 so it can release every child
// before any child starts competing for the CPU.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NCHILD 3
#define NEQUAL 2
#define WORK 20000000
#define WORK_EQ 5000000

static int prios[NCHILD] = {90, 60, 20};
static int rpipe[2]; // children report their completion order here

static void
failed(const char *msg)
{
  printf("t_sched: FAILED: %s\n", msg);
  exit(1);
}

static void
burn(long n)
{
  volatile long x = 0;

  for (long i = 0; i < n; i++)
    x += i;
}

// Start a child that waits for one byte, then burns n iterations and,
// if tag >= 0, writes tag to rpipe. The parent keeps the write end of
// fds so it can release the child later.
static int
start_child(int *fds, int priority, long work, int tag)
{
  int pid;
  char c = 'x';

  if (pipe(fds) < 0)
    failed("pipe failed");
  pid = fork();
  if (pid < 0)
    failed("fork failed");
  if (pid == 0) {
    close(fds[1]);
    if (read(fds[0], &c, 1) != 1)
      failed("child read failed");
    close(fds[0]);
    burn(work);
    if (tag >= 0) {
      if (write(rpipe[1], &tag, sizeof(tag)) != sizeof(tag))
        failed("child write failed");
    }
    exit(0);
  }
  close(fds[0]);
  if (setpriority(pid, priority) != 0)
    failed("setpriority(child) failed");
  if (getpriority(pid) != priority)
    failed("getpriority(child) does not match the value just set");
  return pid;
}

static void
release_child(int *fds)
{
  char c = 'x';

  if (write(fds[1], &c, 1) != 1)
    failed("parent write failed");
  close(fds[1]);
}

static void
reap_all(int n)
{
  int i, status;

  for (i = 0; i < n; i++) {
    if (wait(&status) < 0)
      failed("wait failed");
    if (status != 0)
      failed("a child reported an error");
  }
}

int
main(void)
{
  int fds[NCHILD][2];
  int eqfds[NEQUAL][2];
  int order[NCHILD];
  int i, k, tag;

  if (pipe(rpipe) < 0)
    failed("pipe failed");

  // The parent must outrank the children so that it can release all of
  // them before any child starts burning CPU.
  if (setpriority(getpid(), 100) != 0)
    failed("setpriority(self, 100) failed");

  for (i = 0; i < NCHILD; i++)
    start_child(fds[i], prios[i], WORK, i);
  for (i = 0; i < NCHILD; i++)
    release_child(fds[i]);

  // Read the real completion order from the pipe.
  for (k = 0; k < NCHILD; k++) {
    if (read(rpipe[0], &tag, sizeof(tag)) != sizeof(tag))
      failed("could not read the completion order");
    if (tag < 0 || tag >= NCHILD)
      failed("child sent a bad tag");
    order[k] = prios[tag];
  }
  reap_all(NCHILD);

  for (k = 1; k < NCHILD; k++) {
    if (order[k] >= order[k - 1]) {
      printf("t_sched: finish order: %d %d %d\n",
             order[0], order[1], order[2]);
      failed("children did not finish in descending priority order");
    }
  }
  printf("t_sched: finish order: %d %d %d\n",
         order[0], order[1], order[2]);

  // Equal priorities: both children must eventually run and exit.
  for (i = 0; i < NEQUAL; i++)
    start_child(eqfds[i], 50, WORK_EQ, -1);
  for (i = 0; i < NEQUAL; i++)
    release_child(eqfds[i]);
  reap_all(NEQUAL);

  printf("t_sched: OK\n");
  exit(0);
}
