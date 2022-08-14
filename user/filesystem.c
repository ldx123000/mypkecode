/*
 * The application of lab3_3.
 * parent and child processes never give up their processor during execution.
 */

#include "user/user_lib.h"
#include "util/types.h"
#include "util/string.h"
#include "spike_interface/spike_file.h"


int main(int argc, char *argv[]) {
  int fd;
  int MAXBUF = 512;
  char buf[MAXBUF],buf2[MAXBUF];
  
  printu("\n======== Case 1 ========\n");
  printu("read: \"hostfile.txt\" (from host device)\n");
  printu("========================\n");
  strcpy(buf2,"youyouyou");
  fd = open("user/2.txt", 0);
  printu("file descriptor fd: %d\n", fd);
  write1(fd, buf2, MAXBUF);
  read1(fd, buf, MAXBUF);
  printu("read content: \n%s\n", buf);
  close(fd);

  printu("\n======== Case 2 ========\n");
  printu("write: \"ramdisk0:/ramfile\"\n");
  printu("========================\n");

  fd = open("ramdisk0:/d/ramfile", O_RDWR|O_CREATE);
  printu("file descriptor fd: %d\n", fd);
  write1(fd, buf2, strlen(buf2)+1);
  write1(fd, buf, strlen(buf)+1);
  printu("write content: \n%s\n", buf2);
  close(fd);

  printu("\n======== Case 3 ========\n");
  printu("read: \"ramdisk0:/ramfile\"\n");
  printu("========================\n");
  
  fd = open("ramdisk0:/d/ramfile", O_RDWR);
  printu("file descriptor fd: %d\n", fd);
  read1(fd, buf, MAXBUF);
  printu("read content: \n%s\n", buf);
  close(fd);
  printu("\nAll tests passed!\n\n");

  exit(0);
  return 0;
}
