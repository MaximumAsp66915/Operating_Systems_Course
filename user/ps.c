#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int
main(void)
{
    struct pinfo processes[NPROC];
    int count = getpinfo(processes, NPROC);
    if (count < 0) {
        printf("ps: getpinfo failed\n");
        exit(1);
    }

    printf("PID\tPPID\tAGE\tSTATE\tNAME\n");
    for (int i = 0; i < count; i++) {
        printf("%d\t%d\t%ld\t%s\t%s\n",
               processes[i].pid,
               processes[i].ppid,
               processes[i].age,
               processes[i].state,
               processes[i].name);
    }
    exit(0);
}