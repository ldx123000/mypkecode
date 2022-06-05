/*
 * The application of lab3_3.
 * parent and child processes never give up their processor during execution.
 */

#include "user/user_lib.h"
#include "util/types.h"

int main(void) {
  int a[1010];
  int pid=fork();
  if (pid == 0) {
    printu("Child's va: %ld \n",a);
    printu("Child's pa: %ld \n",showpa((void *)a));
    printu("%d\n",a[0]);
  } else {
    a[0]=2;
    printu("Parent's va: %ld \n",a);
    printu("Parent's pa: %ld \n",showpa((void *)a));
    printu("%d\n",a[0]);
  }
  exit(0);
  return 0;
}
