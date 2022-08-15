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
  int MAXBUF = 503;
  char buf[MAXBUF],buf2[MAXBUF];
  strcpy(buf,"111111");



  printu("\n======== Case 2 ========\n");
  printu("write: \"ramdisk0:/ramfile\"\n");
  printu("========================\n");

  fd = open("Disk_D:/d/ramfile", O_RDWR|O_CREATE);
  printu("file descriptor fd: %d\n", fd);
  //write1(fd, buf2, strlen(buf2)+1);
  write1(fd, buf, strlen(buf)+1);
  printu("write content: \n%s\n", buf);
  close(fd);

  printu("\n======== Case 3 ========\n");
  printu("read: \"ramdisk0:/ramfile\"\n");
  printu("========================\n");
  
  fd = open("Disk_D:/d/ramfile", O_RDWR);
  printu("file descriptor fd: %d\n", fd);
  read1(fd, buf2, MAXBUF);
  printu("read content: \n%s\n", buf);
  close(fd);
  printu("\nAll tests passed!\n\n");

   printu("\n======== Case 0 ========\n");
  printu("write: \"ramdisk0:/ramfile\"\n");
  printu("========================\n");

  fd = open("Disk_1:/d/file", O_RDWR|O_CREATE);
  printu("file descriptor fd: %d\n", fd);
  //write1(fd, buf2, strlen(buf2)+1);
  write1(fd, buf, strlen(buf)+1);
  printu("write content: \n%s\n", buf);
  close(fd);

  printu("\n======== Case 1 ========\n");
  printu("read: \"ramdisk0:/ramfile\"\n");
  printu("========================\n");
  
  fd = open("Disk_1:/d/file", O_RDWR);
  printu("file descriptor fd: %d\n", fd);
  read1(fd, buf2, MAXBUF);
  printu("read content: \n%s\n", buf);
  close(fd);
  printu("\nAll tests passed!\n\n");

  exit(0);
  return 0;
}
