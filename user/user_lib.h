/*
 * header file to be used by applications.
 */
#include "util/types.h"
int printu(const char *s, ...);
int exit(int code);
void* naive_malloc();
void naive_free(void* va);
int fork();
void yield();
int open(const char *path, int flags);
int fgets(void *buf, int fd);
int fputs(void *buf, int fd);
int close(int fd);