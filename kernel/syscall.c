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
  panic( "You need to implement the yield syscall in lab3_2.\n" );

  return 0;
}

//add uart putchar getchar syscall
//
// implement the SYS_user_uart_putchar syscall
//
void sys_user_uart_putchar(uint8 ch) {
    volatile uint32 *status = (void*)(uintptr_t)0x60000008;
    volatile uint32 *tx = (void*)(uintptr_t)0x60000004;
    while (*status & 0x00000008);
    *tx = ch;
}

ssize_t sys_user_uart_getchar() {
  // TODO (lab4_1 and lab4_2): implment the syscall of sys_user_uart_getchar and modify it in lab4_2.
  // hint (lab4_1): the functionality of sys_user_uart_getchar is to get data from UART address. therefore,
  // we should let a pointer point, insert it in 
  // the rear of ready queue, and finally, schedule a READY process to run.
  // hint (lab4_2): the functionality of sys_user_uart_getchar is let process sleep and wait for value. therefore,
  // we should call do_sleep to let process 0 sleep. 
  // then we should get uartvalue and return.
    panic( "You have to implement sys_user_uart_getchar to get data from UART using uartgetchar in lab4_1 and modify it in lab4_2.\n" );
    
}



//car control
ssize_t sys_user_gpio_reg_write(uint8 val) {
    volatile uint32_t *control_reg = (void*)(uintptr_t)0x60001004;
    volatile uint32_t *data_reg = (void*)(uintptr_t)0x60001000;
    //*control_reg = 0;
    *data_reg = (uint32_t)val;
    return 1;
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
    case SYS_user_uart_putchar:
      sys_user_uart_putchar(a1);return 1;
    case SYS_user_uart_getchar:
      return sys_user_uart_getchar();
    case SYS_user_gpio_reg_write:
      return sys_user_gpio_reg_write(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
