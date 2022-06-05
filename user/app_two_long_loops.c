/*
 * The application of lab3_3.
 * parent and child processes never give up their processor during execution.
 */

#include "user/user_lib.h"
#include "util/types.h"
int a[900]={0};
int* b;
int main(void) {
  b=(int *)naive_malloc();
  // int* a=(int *)naive_malloc();
  a[0]=1;
  int pid = fork();
  if (pid == 0) {
    printu("Child's va: %ld \n", a);
    printu("Child's pa: %ld \n", showpa((void *)a));
    printu("%d\n", a[0]);
    a[0]=8;
    printu("Child's va: %ld \n", a);
    printu("Child's pa: %ld \n", showpa((void *)a));
    //printu("Child's va: %ld \n", b);
    //printu("Child's pa: %ld \n", showpa((void *)b));
    printu("%d\n", a[0]);
  } else {
    printu("Parent's va: %ld \n", a);
    printu("Parent's pa: %ld \n", showpa((void *)a));
    printu("%d\n", a[0]);
    a[0] = 2;
    printu("Parent's va: %ld \n", a);
    printu("Parent's pa: %ld \n", showpa((void *)a));
    //printu("Child's va: %ld \n", b);
    //printu("Child's pa: %ld \n", showpa((void *)b));
    printu("%d\n", a[0]);
  }
  exit(0);
  return 0;
}
