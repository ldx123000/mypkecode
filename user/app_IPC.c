/*
 * The application of lab3_challenge3_IPC.
 * This app needs to exchage information between parent and child
 */

#include "user/user_lib.h"
#include "util/types.h"
#define READ 0
#define WRITE 1
#define CHAR_ARR 0
int main(void) {
  int pid = fork();
  int value;
  char *message;
  if (pid == 0) {
    value = 99; // child's value now
    shared_memory(WRITE, (char *)&value, 4, sizeof(int));
    shared_memory(READ, (char *)&value, 0, sizeof(int));
    message = "to parent"; // child's message now
    shared_memory(WRITE, (char *)message, 0, CHAR_ARR);
    shared_memory(READ, (char *)message, 10, CHAR_ARR);
    printu("child's value : %d %s.\n", value, message);
  } else {
    value = -100; // parent's value now
    shared_memory(WRITE, (char *)&value, 0, sizeof(int));
    shared_memory(READ, (char *)&value, 4, sizeof(int));
    message = "to child"; // parent's message now
    shared_memory(WRITE, (char *)message, 10, CHAR_ARR);
    shared_memory(READ, (char *)message, 0, CHAR_ARR);
    printu("parent's value : %d %s.\n", value, message);
  }
  exit(0);
  return 0;
}
