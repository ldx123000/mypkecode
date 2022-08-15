#ifndef _DEV_H_
#define _DEV_H_

#include "riscv.h"
#include "util/types.h"

#define BLOCKS 128       // sum of blocks we define
#define BLOCKSIZE PGSIZE // size of one block we define

//
// device abstract
//
typedef struct device {
  int d_blocks;                             // sum of blocks
  int d_blocksize;                          // size of one block
  int (*d_input)(void *buffer, int blkno);  // device input funtion
  int (*d_output)(void *buffer, int blkno); // device output funtion
} device;

void dev_init(void);

#define dop_input(dev, buffer, blkno) ((dev)->d_input(buffer, blkno))
#define dop_output(dev, buffer, blkno) ((dev)->d_output(buffer, blkno))

// #define dop_open(dev, open_flags)           ((dev)->d_open(dev, open_flags))
// #define dop_close(dev)                      ((dev)->d_close(dev))
//#define dop_io(dev, iob, write)             ((dev)->d_io(dev, iob, write))
// #define dop_ioctl(dev, op, data)            ((dev)->d_ioctl(dev, op, data))

#endif