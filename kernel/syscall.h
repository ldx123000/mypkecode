/*
 * define the syscall numbers of PKE OS kernel.
 */
#ifndef _SYSCALL_H_
#define _SYSCALL_H_

// syscalls of PKE OS kernel. append below if adding new syscalls.
#define SYS_user_base 64
#define SYS_user_print (SYS_user_base + 0)
#define SYS_user_exit (SYS_user_base + 1)
#define SYS_user_allocate_page (SYS_user_base + 2)
#define SYS_user_free_page (SYS_user_base + 3)
#define SYS_user_fork (SYS_user_base + 4)
#define SYS_user_yield (SYS_user_base + 5)

#define SYS_user_get_count (SYS_user_base + 6)
#define SYS_user_set_count (SYS_user_base + 7)
#define SYS_user_init_lock (SYS_user_base + 8)
#define SYS_user_lock (SYS_user_base + 9)
#define SYS_user_unlock (SYS_user_base + 10)


long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7);

#endif
