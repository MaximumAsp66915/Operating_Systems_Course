#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static char msg = 'A';

int
main(void)
{
    int p1[2], p2[2];
    char rec;
    if (pipe(p1) < 0 || pipe(p2) < 0){
        printf("pingpong: Pipe creation failed.\n");
        exit(1);
    }

    int pid = fork();

    if (pid < 0) {
        printf("pingpong: fork failed.\n");
        exit(1);
    }
    if (!pid){
        close(p1[1]);
        close(p2[0]);
        for (int i = 0; i < 1000; i++){
            read(p1[0], &rec, 1);
            write(p2[1], &rec, 1);
        }
        close(p1[0]);
        close(p2[1]);
        exit(0);
    } else {
        close(p1[0]);
        close(p2[1]);
        int s_time = uptime();
        for (int i = 0; i < 1000; i++){
            write(p1[1], &msg, 1);
            read(p2[0], &rec, 1);
        }
        int e_time = uptime();
        int t_time = e_time - s_time;
        printf("pingpong: 1000 exchanges in %d ticks\n", t_time);
        printf("pingpong: %d exchanges/sec\n", 1000 / t_time);
        close(p1[1]);
        close(p2[0]);
        wait(0);
    }
    exit(0);
}