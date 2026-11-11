
// t_killtree: builds a tree of five processes,
// kills the root with killtree(), and verifies that
// every process in the tree is gone.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NNODES 5

static struct pinfo info[64];

static void
failed(const char *msg)
{
  printf("t_killtree: FAILED: %s\n", msg);
  exit(1);
}

static int
in_table(struct pinfo *arr, int n, int pid)
{
  for (int i = 0; i < n; i++)
    if (arr[i].pid == pid)
      return 1;
  return 0;
}

// Print the current process table using getpinfo().
static void
show_processes(char *message)
{
  int n = getpinfo(info, 64);

  printf("\n--- %s ---\n", message);

  if (n < 0)
    failed("getpinfo failed");

  printf("PID\tPPID\tAGE\tSTATE\t\tNAME\n");

  for (int i = 0; i < n; i++) {
    printf("%d\t%d\t%ld\t%s\t\t%s\n",
           info[i].pid,
           info[i].ppid,
           info[i].age,
           info[i].state,
           info[i].name);
  }
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

  printf("t_killtree: Starting test...\n");

  if (pipe(fds) < 0)
    failed("pipe failed");

  printf("t_killtree: Creating process tree...\n");

  root = fork();
  if (root < 0)
    failed("fork failed");

  if (root == 0) {
    int a = fork();
    if (a < 0)
      failed("fork failed");

    if (a == 0) {
      int a1 = fork();
      if (a1 < 0)
        failed("fork failed");

      if (a1 == 0)
        node(fds[1]);

      int a2 = fork();
      if (a2 < 0)
        failed("fork failed");

      if (a2 == 0)
        node(fds[1]);

      node(fds[1]);
    }

    int b = fork();
    if (b < 0)
      failed("fork failed");

    if (b == 0)
      node(fds[1]);

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

  close(fds[0]);

  printf("t_killtree: Process tree created successfully.\n");
  printf("t_killtree: Root PID = %d\n", root);

  for (i = 0; i < NNODES; i++)
    printf("t_killtree: Tree node PID = %d\n", pids[i]);

  // Display the process table before killing the tree.
  show_processes("BEFORE killtree");

  printf("\nt_killtree: Calling killtree(%d)...\n", root);

  if (killtree(root) != 0)
    failed("killtree(root) should return 0");

  printf("t_killtree: killtree returned 0 successfully.\n");

  printf("t_killtree: Testing nonexistent PID...\n");

  if (killtree(999999) != -1)
    failed("killtree of a nonexistent pid should return -1");

  printf("t_killtree: Nonexistent PID test passed.\n");

  printf("t_killtree: Waiting for processes to exit...\n");

  pause(20);
  wait(&status);

  // Check the process table after killing the tree.
  show_processes("AFTER killtree");

  n = getpinfo(info, 64);
  if (n < 0)
    failed("getpinfo returned -1");

  printf("\nt_killtree: Checking all five process IDs...\n");

  for (i = 0; i < NNODES; i++) {
    if (in_table(info, n, pids[i])) {
      printf("t_killtree: PID %d still exists!\n", pids[i]);
      failed("a process of the tree survived killtree");
    }

    printf("t_killtree: PID %d successfully removed.\n", pids[i]);
  }

  printf("\nt_killtree: OK - all processes were terminated.\n");

  exit(0);
}

