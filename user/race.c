#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    int children = 0;
    for (int i = 1; i <= 5; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("race: fork failed\n");
            break;
        }
        if (pid == 0) {
            pause(6 - i);
            exit(i);
        }
        children++;
    }

    for (int i = 0; i < children; i++) {
        int status = 0;
        int child_pid = wait(&status);
        if (child_pid < 0) {
            printf("race: wait failed\n");
            break;
        }
        printf("race: parent reaped child pid %d with status %d\n", child_pid, status);
    }

    printf("race: done\n");
    exit(0);
}