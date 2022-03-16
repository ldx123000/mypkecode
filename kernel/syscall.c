/*
 * contains the implementation of all syscalls.
 */

#include <errno.h>
#include <stdint.h>

#include "pmm.h"
#include "process.h"
#include "string.h"
#include "syscall.h"
#include "util/functions.h"
#include "util/types.h"
#include "vmm.h"

#include "spike_interface/spike_utils.h"

#define SIZE 256
#define MEM_SIZE 5
#define PART_SIZE (PGSIZE/SIZE)

typedef struct arr_part {
  uint64 va;
  int state;
} arr_part;

typedef struct arr_node {
  arr_part part[PART_SIZE];
  uint64 va;
  uint64 state;
} arr_node;

static arr_node mem_used[MEM_SIZE];
static uint8 flag;

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char *buf, size_t n) {
  // buf is an address in user space on user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert(current);
  char *pa = (char *)user_va_to_pa((pagetable_t)(current->pagetable), (void *)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process).
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// maybe, the simplest implementation of malloc in the world ...
//
uint64 sys_user_allocate_page(uint64 nbytes) {
  if (flag == 0) {

    flag = 1;
    for (int i = 0; i < MEM_SIZE; i++) {
      mem_used[i].va = g_ufree_page + i * PGSIZE;
      mem_used[i].state = -1;
      for (int j = 0; j < PART_SIZE; j++) {
        mem_used[i].part[j].va = 0;
        mem_used[i].part[j].state = 0;
      }
    }
  }
  for (int i = 0; i < MEM_SIZE; i++) {
    if (mem_used[i].state > 0&&mem_used[i].state<PART_SIZE) {
      for (int j = 0; j < PART_SIZE; j++) {
        if (mem_used[i].part[j].state == 0) {
          mem_used[i].part[j].va = mem_used[i].va + SIZE * j;
          mem_used[i].part[j].state = 1;
          mem_used[i].state++;
          return mem_used[i].part[j].va;
        }
      }
    }
  }

  void *pa = alloc_page();
  uint64 va = g_ufree_page;
  g_ufree_page += PGSIZE;

  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
              prot_to_type(PROT_WRITE | PROT_READ, 1));
  for (int i = 0; i < MEM_SIZE; i++) {
    if (mem_used[i].state == -1) {
      mem_used[i].va = va;
      mem_used[i].state = 1;
      mem_used[i].part[0].va = va;
      mem_used[i].part[0].state = 1;
    }
  }
  return va;
}

//
// reclaim a page, indicated by "va".
//
uint64 sys_user_free_page(uint64 va) {
  for (int i = 0; i < MEM_SIZE; i++) {
    for (int j = 0; j < PART_SIZE; j++) {
      if (mem_used[i].part[j].va == va && mem_used[i].part[j].state == 1) {
        mem_used[i].part[j].state = 0;
        mem_used[i].state--;
      }
    }
  }
  for (int i = 0; i < MEM_SIZE; i++)
    if (mem_used[i].state == 0) {
      user_vm_unmap((pagetable_t)current->pagetable, mem_used[i].va, PGSIZE, 1);
      mem_used[i].state = -1;
    }

  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
  case SYS_user_print:
    return sys_user_print((const char *)a1, a2);
  case SYS_user_exit:
    return sys_user_exit(a1);
  case SYS_user_allocate_page:
    return sys_user_allocate_page(a1);
  case SYS_user_free_page:
    return sys_user_free_page(a1);
  default:
    panic("Unknown syscall %ld \n", a0);
  }
}
