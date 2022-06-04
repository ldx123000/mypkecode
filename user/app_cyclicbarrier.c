/*
 * This app create two child process.
 * Use cyclicbarrier to control the order of
 * the main process and two child processes print info.
 */
#include "user/user_lib.h"
#include "util/types.h"

int main(void) {
  int pid = fork();
  if (pid == 0) {
    pid = fork();
    if (pid == 0) {
      printu("Process2 print %d\n", 0);
      cyclicbarrier(3);
      printu("Process2 print %d\n", 1);
    } else {
      printu("Process1 print %d\n", 0);
      cyclicbarrier(3);
      printu("Process1 print %d\n", 1);
    }
  } else {
    printu("Process0 print %d\n", 0);
    cyclicbarrier(3);
    printu("Process0 print %d\n", 1);
  }
  exit(0);
  return 0;
}
