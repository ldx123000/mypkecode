/*
 * The application of lab3_challenge5_CopyOnWrite.
 * three processes will share the same data segment until some value changes.
 */

#include "user/user_lib.h"
#include "util/types.h"

int a[1000];

int main(void) {
  a[10] = 100;
  int pid = fork();
  if (pid == 0) {
    pid = fork();
    if (pid == 0) {
      printu("Process2's va: 0x%x \n", a);
      printu("Process2's pa: 0x%x \n", showpa((void *)a));
      printu("Process2's val: %d\n", a[10]);
      a[10] = 2;
      printu("Process2's va: 0x%x \n", a);
      printu("Process2's pa: 0x%x \n", showpa((void *)a));
      printu("Process2's val: %d\n", a[10]);
    } else {
      printu("Process1's va: 0x%x \n", a);
      printu("Process1's pa: 0x%x \n", showpa((void *)a));
      printu("Process1's val: %d\n", a[10]);
      a[10] = 1;
      printu("Process1's va: 0x%x \n", a);
      printu("Process1's pa: 0x%x \n", showpa((void *)a));
      printu("Process1's val: %d\n", a[10]);
    }
  } else {
    printu("Process0's va: 0x%x \n", a);
    printu("Process0's pa: 0x%x \n", showpa((void *)a));
    printu("Process0's val: %d\n", a[10]);
    a[10] = 0;
    printu("Process0's va: 0x%x \n", a);
    printu("Process0's pa: 0x%x \n", showpa((void *)a));
    printu("Process0's val: %d\n", a[10]);
  }
  exit(0);
  return 0;
}
