#include "file.h"
#include "dev.h"
#include "pmm.h"
#include "process.h"
#include "riscv.h"
#include "spike_interface/spike_file.h"
#include "spike_interface/spike_utils.h"
#include "util/functions.h"
#include "util/string.h"
#include "vfs.h"

// ///////////////////////////////////
// Access to the RAM Disk
// ///////////////////////////////////

void fs_init(void) {
  dev_init();
  pfs_init();
}

file *get_file_entry(int fd) {
  file *filep = NULL;
  for (int i = 0; i < MAX_FILES; i++) {
    filep = &(current->filesp->fd_array[i]); // file entry
    if (filep->fd == fd)
      break;
  }
  if (filep == NULL)
    panic("invalid fd!\n");
  return filep;
}

//
// open file
//
int file_open(char *pathname, int flags) {
  // find inode for the file(create if not exist)
  inode *node;
  int ret = vfs_open(pathname, flags, &node);

  // find a free files_structure in the PCB
  int fd;
  struct file *filep = NULL;

  // host device, ret := kfd
  if (ret != 0) {
    fd = ret;
    filep = &(current->filesp->fd_array[fd]);
    filep->status = FD_HOST;
    filep->fd = fd;
  }
  // PKE file's device
  else {
    // find a free file entry in current process
    for (fd = 0; fd < MAX_FILES; fd++) {
      filep = &(current->filesp->fd_array[fd]);
      if (filep->status == FD_NONE)
        break;
    }

    if (filep->status != FD_NONE) // no free entry
      panic("open_fail!\nprobably no file entry for current process!\n");

    //  initialize this file structure
    filep->readable = 0;
    filep->writable = 0;
    filep->status = FD_OPENED;
    filep->fd = fd;
    filep->refcnt = 1;
    filep->node = node;
    // set file's readable & writable
    int readable = 0, writable = 0;
    switch (flags & MASK_FILEMODE) { // get low 2 bits
    case O_RDONLY:
      filep->readable = 1;
      break;
    case O_WRONLY:
      filep->writable = 1;
      break;
    case O_RDWR:
      filep->readable = 1;
      filep->writable = 1;
      break;
    default:
      panic("invalid open flags!\n");
    }

    // use vop_fstat to get the file size from inode, and set the offset
    filep->off = 0;
    struct fstat st; // file status
    if ((ret = vop_fstat(node, &st)) != 0) {
      panic("do_ram_open: failed to get file status!\n");
    }
    filep->off = st.st_size;
  }

  ++current->filesp->files_count;
  return filep->fd;
}

//
// find file entry by fd
// and read
//
int file_read(int fd, char *buf, uint64 count) {
  file *filep = get_file_entry(fd);

  // host file
  if (filep->status == FD_HOST) {
    sys_read(fd, buf, count);
    return 0;
  }

  // PKE file
  if (filep->readable == 0) // 如果不可读
    panic("unreadable file!\n");

  char buffer[count + 1];

  vop_read(filep->node, buffer, count);

  strcpy(buf, buffer);

  return 0;
}

//
// find file entry by fd
// and write
//
int file_write(int fd, char *buf, uint64 count) {
  struct file *filep = get_file_entry(fd);

  // host file
  if (filep->status == FD_HOST) {
    sys_write(fd, buf, count);
    return 0;
  }

  // PKE file
  if (filep->writable == 0) // 如果不可读
    panic("unwritable file!\n");

  vop_write(filep->node, buf, count);

  return 0;
}

//
// close file
//
int file_close(int fd) {
  struct file *filep = get_file_entry(fd);
  // host file
  if (filep->status == FD_HOST) {
    sys_close(fd);
  } else {
    --filep->node->ref_count;
  }
  filep->status = FD_NONE;
  return 0;
}

//
//  initialize a files_struct for a process
//
files_struct *files_struct_init(void) {
  files_struct *filesp = (files_struct *)alloc_page();
  filesp->pwd = NULL; // 将进程打开的第一个文件的目录作为进程的cwd
  filesp->files_count = 0;
  // save file entries for spike files
  for (int i = 0; i < MAX_FILES; ++i) {
    if (spike_files[i].kfd == -1) {
      filesp->fd_array[i].status = FD_NONE;
      filesp->fd_array[i].refcnt = 0;
    } else {
      filesp->fd_array[i].status = FD_HOST;
      filesp->fd_array[i].fd = spike_files[i].kfd;
      filesp->fd_array[i].refcnt = spike_files[i].refcnt;
      ++filesp->files_count;
    }
  }
  sprint("FS: create a files_struct for process: files_count: %d\n", filesp->files_count);
  return filesp;
}

//
// destroy a files_struct for a process
//
void files_destroy(struct files_struct *filesp) {
  free_page(filesp);
  return;
}

int sys_open(char *path, int flags) {
  spike_file_t *f = spike_file_open(path, flags, 0);
  int fd = spike_file_dup(f);
  return fd;
}

int sys_read(int fd, char *buf, uint64 size) {
  spike_file_t *f = spike_file_get(fd);
  return spike_file_read(f, buf, size);
}

int sys_write(int fd, char *buf, uint64 size) {
  spike_file_t *f = spike_file_get(fd);
  return spike_file_write(f, buf, size);
}

int sys_close(int fd) {
  return spike_fd_close(fd);
}