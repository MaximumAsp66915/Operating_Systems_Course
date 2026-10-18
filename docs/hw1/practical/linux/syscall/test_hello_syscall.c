#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_hello_syscall 451

int main(void)
{
	long ret;

	printf("Calling hello_syscall...\n");
	ret = syscall(__NR_hello_syscall);
	printf("hello_syscall returned: %ld\n", ret);
	return 0;
}
