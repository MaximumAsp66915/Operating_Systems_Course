// t_killtree: builds a tree of five processes, kills the root with
// killtree(), and checks that every process in the tree is gone.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NNODES 5

static void
failed(const char *msg)
{
  printf("t_killtree: FAILED: %s\n", msg);
  exit(1);
}

static int
in_table(struct pinfo *arr, int n, int pid)
{
  int i;
  for (i = 0; i < n; i++)
    if (arr[i].pid == pid)
      return 1;
  return 0;
}

static void
node(int wfd)
{
  int pid = getpid();
  write(wfd, &pid, sizeof(int));
  pause(10000);
  exit(0);
}

int
main(int argc, char *argv[])
{
  int fds[2];
  int pids[NNODES];
  int root, i, n, status;
  struct pinfo info[64];

  if (pipe(fds) < 0)
    failed("pipe failed");

  root = fork();
  if (root < 0)
    failed("fork failed");
  if (root == 0) {
    // Root of the tree: two children a and b. The root's pid is
    // already known to the parent, so only the other four nodes
    // report their pid through the pipe.
    int a = fork();
    if (a < 0)
      failed("fork failed");
    if (a == 0) {
      // a has two children of its own: a1 and a2.
      int a1 = fork();
      if (a1 < 0)
        failed("fork failed");
      if (a1 == 0) {
        node(fds[1]);
      }
      int a2 = fork();
      if (a2 < 0)
        failed("fork failed");
      if (a2 == 0) {
        node(fds[1]);
      }
      node(fds[1]);
    }
    int b = fork();
    if (b < 0)
      failed("fork failed");
    if (b == 0) {
      node(fds[1]);
    }
    pause(10000);
    exit(0);
  }

  close(fds[1]);
  pids[0] = root;
  for (i = 1; i < NNODES; i++) {
    int got = read(fds[0], &pids[i], sizeof(int));
    if (got != sizeof(int))
      failed("could not read a pid from the pipe");
  }

  // Every node now exists; kill the whole tree.
  if (killtree(root) != 0)
    failed("killtree(root) should return 0");
  if (killtree(999999) != -1)
    failed("killtree of a nonexistent pid should return -1");

  // Give the killed processes time to exit and be reaped.
  pause(20);
  wait(&status);

  n = getpinfo(info, 64);
  if (n < 0)
    failed("getpinfo returned -1");
  for (i = 0; i < NNODES; i++) {
    if (in_table(info, n, pids[i])) {
      printf("t_killtree: pid %d still alive\n", pids[i]);
      failed("a process of the tree survived killtree");
    }
  }

  printf("t_killtree: OK\n");
  exit(0);
}
