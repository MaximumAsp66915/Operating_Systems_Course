#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int a, b;

  if(argc != 3){
    fprintf(2, "usage: power a b\n");
    exit(1);
  }
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  printf("%d^%d = %d\n", a, b, power(a, b));
  exit(0);
}
