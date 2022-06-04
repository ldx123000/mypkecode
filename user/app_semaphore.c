/*
 * This app create two child process.
 * Use semaphores to control the order of
 * the main process and two child processes print info.
 */
#include "user/user_lib.h"
#include "util/types.h"

int main(void) {
  int pid = fork();
  if (pid == 0) { //子
    pid = fork();
    if (pid == 0) {
      printu("grandson print %d\n", 0);
      cyclicbarrier(3);
      printu("grandson print %d\n", 1);
    } else {
      printu("son print %d\n", 0);
      cyclicbarrier(3);
      printu("son print %d\n", 1);
    }
    // for (int i = 0; i < 10; i++) {
    //   sem_P(child_sem[pid == 0]); // child_sem[0 or 1]
    //   printu("Child%d print %d\n", pid == 0, i);
    //   if (pid != 0)//child 1
    //     sem_V(child_sem[1]);
    //   else//child 0
    //     sem_V(main_sem);
    // }
  } else { //父
    printu("Parent print %d\n", 0);
    cyclicbarrier(3);
    pp();
    // for (int i = 0; i < 10; i++) {
    //   sem_P(main_sem);
    //   printu("Parent print %d\n", i);
    //   sem_V(child_sem[0]);
    // }
  }
  exit(0);
  return 0;
}
