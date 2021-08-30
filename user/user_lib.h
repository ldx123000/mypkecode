/*
 * header file to be used by applications.
 */

int printu(const char *s, ...);
int exit(int code);
void* naive_malloc();
void naive_free(void* va);
int fork();
void yield();
//add
int uartputchar(char ch);
int uartgetchar();

int gpio_reg_write(char val);
