/*
 * Supervisor-mode startup codes
 */

#include "elf.h"
#include "filesystem.h"
#include "memlayout.h"
#include "pmm.h"
#include "process.h"
#include "riscv.h"
#include "sched.h"
#include "spike_interface/spike_utils.h"
#include "string.h"
#include "vmm.h"

//
// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point of
// S-mode trap vector).
//
extern char trap_sec_start[];

//
// turn on paging.
//
void enable_paging() {
  // write the pointer to kernel page (table) directory into the CSR of "satp".
  write_csr(satp, MAKE_SATP(g_kernel_pagetable));

  // refresh tlb to invalidate its content.
  flush_tlb();
}

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
process *load_user_program() {
  process *proc;

  proc = alloc_process();

  sprint("User application is loading.\n");
  load_bincode_from_host_elf(proc);

  return proc;
}

//
// s_start: S-mode entry point of PKE OS kernel.
//
int s_start(void) {
  sprint("Enter supervisor mode...\n");
  // in the beginning, we use Bare mode (direct) memory mapping as in lab1,
  // but now switch to paging mode in lab2.
  write_csr(satp, 0);

  // init phisical memory manager
  pmm_init();

  // build the kernel page table
  kern_vm_init();

  // now, switch to paging mode by turning on paging (SV39)
  enable_paging();
  sprint("kernel page table is on \n");

  init_proc_pool();

  // then init file system
  fs_init();
  sprint("initial file system complete!\n");

  // the application code (elf) is first loaded into memory, and then put into execution
  sprint("Switch to user mode...\n");
  insert_to_ready_queue(load_user_program());
  schedule();

  return 0;
}
