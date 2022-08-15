#include "dev.h"
#include "pmm.h"
#include "riscv.h"
#include "spike_interface/spike_utils.h"
#include "util/functions.h"
#include "util/string.h"
#include "util/types.h"
#include "vfs.h"

// DISK BASE ADDRESS list
void *DISK_BASE_ADDR[MAX_DEV];
int index = 0;

//
// load data from disk
//
void dev_data_load(char *devname, device *dev) {
  int fd = sys_open(devname, 0);
  char buf[BLOCKSIZE];
  for (int blkno = 0; blkno < BLOCKS; blkno++) {
    sys_read(fd, buf, BLOCKSIZE);
    void *dst = (void *)((uint64)DISK_BASE_ADDR[dev->d_no] + blkno * BLOCKSIZE);
    memcpy(dst, (void *)buf, BLOCKSIZE);
  }
  return;
}

//
// init device add into vfs_dev list
//
void dev_disk_init(char *devname) {
  // alloc space for new disk
  for (int i = ROUNDUP(BLOCKS * BLOCKSIZE, PGSIZE) / PGSIZE; i > 0; i--) {
    DISK_BASE_ADDR[index] = alloc_page();
  }

  vfs_dev_t *pdev = (vfs_dev_t *)alloc_page();

  // set device struct
  struct device *pd = (struct device *)alloc_page();
  pd->d_blocks = BLOCKS;
  pd->d_blocksize = BLOCKSIZE;
  pd->d_no = index;
  pd->d_io = disk_io;

  // set device info
  pdev->devname = devname;
  pdev->dev = pd;

  // load data from disk if need
  // dev_data_load(devname,pd);

  // add device to vfs_dev list
  vdev_list[index++] = pdev;
  sprint("base address of RAM Disk0 is: %p\n", DISK_BASE_ADDR[index]);
}

//
// Initialize devices
//
void dev_init(void) {
  dev_disk_init("ramdisk0"); // add the device entry to vfs_dev_list
}

int disk_io(device *dev, int blkno, void *iob, bool write) {
  if (blkno < 0 || blkno >= BLOCKS)
    panic("block number out of range!\n");
  if (write == 0) { // read: data from block to buffer
    void *src = (void *)((uint64)DISK_BASE_ADDR[dev->d_no] + blkno * BLOCKSIZE);
    memcpy(iob, src, BLOCKSIZE);
  } else { // write: data from buffer to block
    void *dst = (void *)((uint64)DISK_BASE_ADDR[dev->d_no] + blkno * BLOCKSIZE);
    memcpy(dst, iob, BLOCKSIZE);
  }
  return 0;
}

void dev_end(char *devname, device *dev) {
  int fd = sys_open(devname, 0);
  char buf[BLOCKSIZE];
  for (int blkno = 0; blkno < BLOCKS; blkno++) {
    void *src = (void *)((uint64)DISK_BASE_ADDR[dev->d_no] + blkno * BLOCKSIZE);
    memcpy((void *)buf, src, BLOCKSIZE);
    sys_write(fd, buf, BLOCKSIZE);
  }
  return;
}

//
// Function table for device inodes.
//
// static const struct inode_ops dev_node_ops = {
//   .vfs_open                       = dev_open,
//   .vfs_close                      = dev_close,
//   .vfs_read                       = dev_read,
//   .vfs_write                      = dev_write,
//   .vfs_fstat                      = dev_fstat,
//   .vfs_ioctl                      = dev_ioctl,
//   .vfs_gettype                    = dev_gettype,
//   .vfs_tryseek                    = dev_tryseek,
//   .vfs_lookup                     = dev_lookup,
// };

// //
// // Create an inode for device
// //
// struct inode *dev_create_inode(void){
//   struct inode * node = alloc_inode(RAMDISK0, T_DEV, &dev_node_ops, NULL);
//   return node;
// }
