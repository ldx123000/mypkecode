/*
 * The supporting library for applications.
 * Actually, supporting routines for applications are catalogued as the user
 * library. we don't do that in PKE to make the relationship between application
 * and user library more straightforward.
 */

#include "user_lib.h"
#include "kernel/syscall.h"
#include "util/snprintf.h"
#include "util/types.h"

uint64 do_user_call(uint64 sysnum, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6,
                    uint64 a7) {
  int ret;

  // before invoking the syscall, arguments of do_user_call are already loaded into the argument
  // registers (a0-a7) of our (emulated) risc-v machine.
  asm volatile(
      "ecall\n"
      "sw a0, %0" // returns a 32-bit value
      : "=m"(ret)
      :
      : "memory");

  return ret;
}

//
// printu() supports user/lab1_1_helloworld.c
//
int printu(const char *s, ...) {
  va_list vl;
  va_start(vl, s);

  char out[256]; // fixed buffer size.
  int res = vsnprintf(out, sizeof(out), s, vl);
  va_end(vl);
  const char *buf = out;
  size_t n = res < sizeof(out) ? res : sizeof(out);

  // make a syscall to implement the required functionality.
  return do_user_call(SYS_user_print, (uint64)buf, n, 0, 0, 0, 0, 0);
}

//
// applications need to call exit to quit execution.
//
int exit(int code) {
  return do_user_call(SYS_user_exit, code, 0, 0, 0, 0, 0, 0);
}

//
// lib call to naive_malloc
//
void *naive_malloc() {
  return (void *)do_user_call(SYS_user_allocate_page, 0, 0, 0, 0, 0, 0, 0);
}

//
// lib call to naive_free
//
void naive_free(void *va) {
  do_user_call(SYS_user_free_page, (uint64)va, 0, 0, 0, 0, 0, 0);
}

//
// lib call to naive_fork
int fork() {
  return do_user_call(SYS_user_fork, 0, 0, 0, 0, 0, 0, 0);
}

//
// lib call to yield
//
void yield() {
  do_user_call(SYS_user_yield, 0, 0, 0, 0, 0, 0, 0);
}

int getCount() {
  return do_user_call(SYS_user_get_count, 0, 0, 0, 0, 0, 0, 0);
}

void setCount(int value) {
  do_user_call(SYS_user_set_count, value, 0, 0, 0, 0, 0, 0);
}

int init_lock(char *name) {
  return do_user_call(SYS_user_init_lock, (uint64)name, 0, 0, 0, 0, 0, 0);
}

void lock(int lock) {
  do_user_call(SYS_user_lock, lock, 0, 0, 0, 0, 0, 0);
}

void unlock(int lock) {
  do_user_call(SYS_user_unlock, lock, 0, 0, 0, 0, 0, 0);
}

void interval() {
  uint64 rounds = 30000000;
  uint64 interval = 10000000;
  for (uint64 i = 0; i < rounds; ++i) {
    if (i % interval == 0)
      printu("running %ld \n", i);
  }
}

int atomCount() {
  int tmp = getCount();
  interval();
  tmp = tmp + 1;
  interval();
  setCount(tmp);
  return tmp;
}

void cyclicbarrier(int total) {
  int countlock = init_lock("countlock");
  lock(countlock);
  atomCount();
  unlock(countlock);
  while (getCount() != total)
    ;
  return;
}