#include "kernel/types.h"
#include "user/user.h"

#define NCHILD 5
#define ITERATIONS 1000000

int
main(void)
{
    int gates[NCHILD][2];
    int i, j;
    char token = 'x';
    int parent_pid = getpid();
    int start, finish;

    for (i = 0; i < NCHILD; i++) {
        if (pipe(gates[i]) < 0) {
            printf("priority_test: pipe failed\n");
            exit(1);
        }
    }

    for (i = 0; i < NCHILD; i++) {
        int pid = fork();

        if (pid < 0) {
            printf("priority_test: fork failed\n");
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

            if (read(gates[i][0], &token, 1) != 1) {
                exit(1);
            }
            close(gates[i][0]);

            printf("priority_test: child %d pid=%d priority=%d starts\n",
                   i + 1, getpid(), getpriority(getpid()));

            volatile int counter = 0;
            for (int k = 0; k < ITERATIONS; k++) {
                counter++;
            }

            printf("priority_test: child %d pid=%d finished, counter=%d\n",
                   i + 1, getpid(), counter);
            exit(0);
        }

        close(gates[i][0]);

        if (setpriority(pid, (i + 1) * 10) != 0) {
            printf("priority_test: setpriority failed for pid=%d\n", pid);
            exit(1);
        }

        printf("priority_test: pid=%d assigned priority=%d, readback=%d\n",
               pid, (i + 1) * 10, getpriority(pid));
    }

    if (setpriority(parent_pid, 100) != 0) {
        printf("priority_test: cannot raise parent priority\n");
        exit(1);
    }

    start = uptime();
    for (i = 0; i < NCHILD; i++) {
        if (write(gates[i][1], &token, 1) != 1) {
            printf("priority_test: gate write failed\n");
            exit(1);
        }
        close(gates[i][1]);
    }

    if (setpriority(parent_pid, 0) != 0) {
        printf("priority_test: cannot restore parent priority\n");
        exit(1);
    }

    printf("priority_test: all children released; parent priority=%d\n",
           getpriority(parent_pid));

    for (i = 0; i < NCHILD; i++) {
        int status;
        int finished_pid = wait(&status);
        printf("priority_test: wait returned pid=%d status=%d\n",
               finished_pid, status);
    }

    finish = uptime();
    printf("priority_test: total elapsed time=%d ticks\n", finish - start);

    if (setpriority(2147483647, 50) == -1 &&
        getpriority(2147483647) == -1) {
        printf("priority_test: invalid PID handling passed\n");
    } else {
        printf("priority_test: invalid PID handling FAILED\n");
    }

    exit(0);
}
