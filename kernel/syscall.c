/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"

#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  //buf is an address in user space on user stack,
  //so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab3 now, we should reclaim the current process, and reschedule.
  free_process( current );
  schedule();
  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ...
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  uint64 va = g_ufree_page;
  g_ufree_page += PGSIZE;
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va".
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current );
}

//
// kerenl entry point of yield
//
ssize_t sys_user_yield() {
  // TODO (lab3_2): implment the syscall of yield.
  // hint: the functionality of yield is to give up the processor. therefore,
  // we should set the status of currently running process to READY, insert it in 
  // the rear of ready queue, and finally, schedule a READY process to run.
  current->status=READY;
  insert_to_ready_queue(current);
  schedule();
  //panic( "You need to implement the yield syscall in lab3_2.\n" );

  return 0;
}

ssize_t sys_user_get_count() {
   return getCount();
}

ssize_t sys_user_set_count(uint64 va) {
   setCount(va);
   return 0;
}

uint64 sys_user_init_lock(char *name) {
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)name);
  return init_lock(pa);
}


ssize_t sys_user_lock(int sl) {
  lock(sl);
  return 0;
}

ssize_t sys_user_unlock(int sl) {
  unlock(sl);
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_allocate_page:
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    case SYS_user_fork:
      return sys_user_fork();
    case SYS_user_yield:
      return sys_user_yield();
    case SYS_user_get_count:
      return sys_user_get_count();
    case SYS_user_set_count:
      return sys_user_set_count(a1);
    case SYS_user_init_lock:
      return sys_user_init_lock((char*)a1);
    case SYS_user_lock:
      return sys_user_lock(a1);
    case SYS_user_unlock:
      return sys_user_unlock(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
