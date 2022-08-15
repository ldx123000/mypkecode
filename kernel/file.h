#ifndef _FILE_H_
#define _FILE_H_

#include "process.h"
#include "spike_interface/spike_file.h"
#include "util/types.h"

#define MASK_FILEMODE 0x003

// //////////////////////////////////////////////////
// File operation interfaces provided to the process
// //////////////////////////////////////////////////

int file_open(char *pathname, int flags);
int file_read(int fd, char *buf, uint64 count);
int file_write(int fd, char *buf, uint64 count);
int file_close(int fd);

// ///////////////////////////////////
// Access to the RAM Disk
// ///////////////////////////////////

void fs_init(void);

typedef struct file {
  enum {
    FD_HOST,
    FD_NONE,
    FD_OPENED,
    FD_CLOSED,
  } status;           // file status
  bool readable;      // read
  bool writable;      // write
  int fd;             // file descriptor
  int off;            // offset
  int refcnt;         // reference count
  struct inode *node; // inode
}file;

struct fstat {
  int st_mode;   // protection mode and file type
  int st_nlinks; // # of hard links
  int st_blocks; // # of blocks file is using
  int st_size;   // file size (bytes)
};

// void fd_array_init(struct file *fd_array);
// void fd_array_open(struct file *file);
// void fd_array_close(struct file *file);
// void fd_array_dup(struct file *to, struct file *from);
// bool file_testfd(int fd, bool readable, bool writable);

// current running process
extern process *current;

typedef struct files_struct {
  struct inode *pwd;               // inode of present working directory
  struct file fd_array[MAX_FILES-1]; // opened files array
  int files_count;                 // the number of opened files
} files_struct;

files_struct *files_struct_init(void);
void files_destroy(struct files_struct *filesp);

int sys_open(char *pathname, int flags);
int sys_read(int fd, char *buf, uint64 count);
int sys_write(int fd, char *buf, uint64 count);
int sys_close(int fd);

#endif
