#include "kernel/types.h"
#include "user/user.h"

#define NCHILD 2
#define ITERATIONS 1000000

int
main(void)
{
    int gates[NCHILD][2];
    int priorities[NCHILD] = {20, 80};
    int parent_pid = getpid();
    int i, j;
    char token = 's';

    for (i = 0; i < NCHILD; i++) {
        if (pipe(gates[i]) < 0) {
            printf("setpriority_test: pipe failed\n");
            exit(1);
        }
    }

    for (i = 0; i < NCHILD; i++) {
        int pid = fork();

        if (pid < 0) {
            printf("setpriority_test: fork failed\n");
            exit(1);
        }

        if (pid == 0) {
            for (j = 0; j < NCHILD; j++) {
                if (j != i) {
                    close(gates[j][0]);
                    close(gates[j][1]);
                }
            }
            close(gates[i][1]);

            // Each child remains blocked until the parent has configured it.
            if (read(gates[i][0], &token, 1) != 1) {
                exit(1);
            }
            close(gates[i][0]);

            printf("setpriority_test: child %d pid=%d priority=%d starts\n",
                   i + 1, getpid(), getpriority(getpid()));

            volatile int counter = 0;
            for (int k = 0; k < ITERATIONS; k++) {
                counter++;
            }

            printf("setpriority_test: child %d pid=%d finished, counter=%d\n",
                   i + 1, getpid(), counter);
            exit(0);
        }

        close(gates[i][0]);

        if (setpriority(pid, priorities[i]) != 0) {
            printf("setpriority_test: cannot set priority for pid=%d\n", pid);
            exit(1);
        }

        printf("setpriority_test: child pid=%d priority=%d\n",
               pid, getpriority(pid));
    }

    printf("setpriority_test: parent's initial priority=%d\n",
           getpriority(parent_pid));

    // Demonstrate clamping at both ends of the allowed interval.
    setpriority(parent_pid, -10);
    printf("setpriority_test: set -10; clamped priority=%d (expected 0)\n",
           getpriority(parent_pid));

    // Raise the currently running parent. It now outranks both children.
    setpriority(parent_pid, 150);
    printf("setpriority_test: set 150; clamped priority=%d (expected 100)\n",
           getpriority(parent_pid));

    // Release both children while the parent has priority 100.
    for (i = 0; i < NCHILD; i++) {
        if (write(gates[i][1], &token, 1) != 1) {
            printf("setpriority_test: gate write failed\n");
            exit(1);
        }
        close(gates[i][1]);
    }

    printf("setpriority_test: parent is doing CPU work at priority=%d\n",
           getpriority(parent_pid));

    volatile int parent_counter = 0;
    for (i = 0; i < ITERATIONS; i++) {
        parent_counter++;
    }

    // Lowering the parent's priority allows child 2 (priority 80) to run
    // before child 1 (priority 20).
    setpriority(parent_pid, -1);
    printf("setpriority_test: parent lowered to priority=%d; children can run\n",
           getpriority(parent_pid));

    for (i = 0; i < NCHILD; i++) {
        int status;
        int finished_pid = wait(&status);
        printf("setpriority_test: wait returned pid=%d status=%d\n",
               finished_pid, status);
    }

    printf("setpriority_test: parent work counter=%d\n", parent_counter);
    exit(0);
}
