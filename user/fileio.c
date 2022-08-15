/*
 * Below is the given application for lab5_3.
 * The goal of this app is to get correct information by using function fputs,fgets. 
 */

#include "user/user_lib.h"
#include "util/types.h"
#include "util/string.h"
#include "spike_interface/spike_file.h"


int main(int argc, char *argv[]) {
  int fd;
  char buf[64],buf2[64];
  strcpy(buf,"this is a message!\n");

  fd = open("Disk_D:/files", O_RDWR|O_CREATE);
  fputs(buf, fd);
  printu("write content: \n%s\n", buf);
  close(fd);

  fd = open("Disk_D:/files", O_RDONLY);
  fgets(buf2, fd);
  printu("read content: \n%s\n", buf);
  close(fd);

  strcpy(buf,"hello world!\n");
  fd = open("Disk_1:/d/file", O_RDWR|O_CREATE);
  fputs(buf, fd);
  printu("write content: \n%s\n", buf);
  close(fd);

  fd = open("Disk_1:/d/file", O_RDONLY);
  fgets(buf2, fd);
  printu("read content: \n%s\n", buf);
  close(fd);

  exit(0);
  return 0;
}