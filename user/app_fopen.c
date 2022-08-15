 /*
 * Below is the given application for lab5_2.
 * The goal of this app is to get correct information by using function open. 
 */

#include "user/user_lib.h"
#include "util/types.h"
#include "util/string.h"
#include "spike_interface/spike_file.h"


int main(int argc, char *argv[]) {
  int fd;

  fd = open("Disk_D:/a/b/c/files", O_RDWR|O_CREATE);
  printu("file descriptor is %d\n\n",fd);

  exit(0);
  return 0;
}
