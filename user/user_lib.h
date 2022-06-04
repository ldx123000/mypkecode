/*
 * header file to be used by applications.
 */

int printu(const char *s, ...);
int exit(int code);
void* naive_malloc();
void naive_free(void* va);
int fork();
void yield();

int getCount();
void setCount(int value);
int atomCount();

int init_lock(char *name);
void lock(int lock);
void unlock(int lock);

void cyclicbarrier(int total);