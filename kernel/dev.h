#ifndef _DEV_H_
#define _DEV_H_

#include "riscv.h"
#include "util/types.h"

#define BLOCKS 64       // sum of blocks we define
#define BLOCKSIZE PGSIZE // size of one block we define

//
// device abstract
//
typedef struct device {
  int d_blocks;                                                      // sum of blocks
  int d_blocksize;                                                   // size of one block
  int d_no;                                                          // device number
  int (*d_io)(struct device *dev, int blkno, void *iob, bool write); // device io
} device;

void dev_init(void);
int disk_io(device *dev, int blkno, void *iob, bool write);
void dev_end(char *devname, device *dev);

// #define dop_open(dev, open_flags)           ((dev)->d_open(dev, open_flags))
// #define dop_close(dev)                      ((dev)->d_close(dev))
#define dev_op_io(dev, blkno, iob, write) ((dev)->d_io(dev, blkno, iob, write))
// #define dop_ioctl(dev, op, data)            ((dev)->d_ioctl(dev, op, data))

#endif