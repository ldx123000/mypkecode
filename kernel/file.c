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
  rfs_init();
}

// //////////////////////////////////////////////////
// File operation interfaces provided to the process
// //////////////////////////////////////////////////

//
// open file
//
int file_open(char *pathname, int flags) {
  // set file's readable & writable
  int readable = 0, writable = 0;
  switch (flags & MASK_FILEMODE) { // get low 2 bits
  case O_RDONLY:
    readable = 1;
    break;
  case O_WRONLY:
    writable = 1;
    break;
  case O_RDWR:
    readable = 1;
    writable = 1;
    break;
  default:
    panic("do_ram_open: invalid open flags!\n");
  }

  // find inode for the file in pathname (create if not exist)
  struct inode *node;
  int ret;
  ret = vfs_open(pathname, flags, &node);

  // find a free files_structure in the PCB
  int fd;
  struct file *pfile;

  // 2.1. Case 1: host device, ret := kfd
  if (ret != 0) {
    fd = ret;
    pfile = &(current->filesp->fd_array[fd]);
    pfile->status = FD_HOST;
    pfile->fd = fd;
  }
  // 2.2. Case 2: PKE device
  else {
    // find a free file entry
    for (fd = 0; fd < MAX_FILES; fd++) {
      pfile = &(current->filesp->fd_array[fd]); // file entry
      if (pfile->status == FD_NONE)          // find a free entry
        break;
    }
    if (pfile->status != FD_NONE) // no free entry
      panic("do_ram_open: no file entry for current process!\n");

    // initialize this file structure
    pfile->status = FD_OPENED;
    pfile->readable = readable;
    pfile->writable = writable;
    pfile->fd = fd;
    pfile->refcnt = 1;
    pfile->node = node;

    // use vop_fstat to get the file size from inode, and set the offset
    pfile->off = 0;
    struct fstat st; // file status
    if ((ret = vop_fstat(node, &st)) != 0) {
      panic("do_ram_open: failed to get file status!\n");
    }
    pfile->off = st.st_size;
  }

  ++current->filesp->files_count;
  return pfile->fd;
}

int file_read(int fd, char *buf, uint64 count) {
  // 根据 fd 找到 file
  struct file *pfile = NULL;

  for (int i = 0; i < MAX_FILES; ++i) {
    pfile = &(current->filesp->fd_array[i]); // file entry
    if (pfile->fd == fd)
      break;
  }
  if (pfile == NULL)
    panic("do_read: invalid fd!\n");

  // 打开宿主机文件
  if (pfile->status == FD_HOST) {
    host_read(fd, buf, count);
    return 0;
  }

  // 打开PKE Device文件
  if (pfile->readable == 0) // 如果不可读
    panic("do_read: no readable file!\n");

  char buffer[count + 1];

  vop_read(pfile->node, buffer, count);

  strcpy(buf, buffer);

  return 0;
}

int file_write(int fd, char *buf, uint64 count) {
  // 根据 fd 找到 file
  struct file *pfile = NULL;

  for (int i = 0; i < MAX_FILES; ++i) {
    pfile = &(current->filesp->fd_array[i]); // file entry
    if (pfile->fd == fd)
      break;
  }
  if (pfile == NULL)
    panic("do_write: invalid fd!\n");

  // 打开宿主机文件
  if (pfile->status == FD_HOST) {
    host_write(fd, buf, count);
    return 0;
  }
  sprint("%d\n", fd);
  // 打开PKE Device文件
  if (pfile->writable == 0) // 如果不可读
    panic("do_write: cannot write file!\n");

  vop_write(pfile->node, buf, count);

  return 0;
}

int file_close(int fd) {
  // 根据 fd 找到 file
  struct file *pfile = NULL;

  for (int i = 0; i < MAX_FILES; ++i) {
    pfile = &(current->filesp->fd_array[i]); // file entry
    if (pfile->fd == fd)
      break;
  }
  if (pfile == NULL)
    panic("do_write: invalid fd!\n");

  // 关闭宿主机文件
  if (pfile->status == FD_HOST) {
    host_close(fd);
  } else {
    --pfile->node->ref;
  }

  pfile->status = FD_NONE;
  return 0;
}

// ///////////////////////////////////
// Files struct in PCB
// ///////////////////////////////////

/*
 * initialize a files_struct for a process
 * struct files_struct * filesp:
 *      pwd:      current working directory
 *      ofile:    array of file structure
 *      files_count:    * of opened files for current process
 */
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
