//PKE file system 
//specific file system
#ifndef _pfs_H_
#define _pfs_H_

#include "riscv.h"
#include "util/functions.h"
#include "util/types.h"
#include "vfs.h"

#define PFS_TYPE 0
#define PFS_MAGIC 0x2f8dbe2a
#define PFS_BLKSIZE PGSIZE
#define PFS_BLKN_SUPER 0  // 1 for superblock
#define PFS_BLKN_BITMAP 1 // 1 for bitmap
#define PFS_BLKN_DIN 2  // 16 for inodes
#define PFS_MAX_DIN_NUM 16
#define PFS_BLKN_FREE 18
#define PFS_MAX_FNAME_LEN 60
#define PFS_NDIRECT 16 //each disk_inode have 16

// pfs block number

struct fs;
struct inode;
struct file;

// file system super block
typedef struct super {
  int magic;   // magic number of the
  int size;    // Size of file system image (blocks)
  int nblocks; // Number of data blocks
  int ninodes; // Number of inodes.
} super;

// filesystem for pfs
typedef struct pfs_fs {
  struct super super; // pfs_superblock
  struct device *dev; // device mounted on
  int *freemap;       // marked 1 if blocks in use
  bool dirty;         // true if super/freemap modified
  void *buffer;       // buffer for io
} pfs_fs;

// inode on disk
typedef struct disk_inode {
  int size;                   // size of the file (bytes)
  int type;                   // T_FREE, T_DEV, T_FILE, T_DIR
  int nlinks;                 // number of hard links
  int blocks;                 // number of blocks is using
  uint64 direct[PFS_NDIRECT]; // directory blocks
} disk_inode;

// directory entry
typedef struct pfs_dir_entry {
  int inum;                     // inode number
  char name[PFS_MAX_FNAME_LEN]; // file name
} pfs_dir_entry;

void pfs_init(void);
int pfs_mount(const char *devname);
int pfs_do_mount(struct device *dev, struct fs **vfs_fs);
int pfs_unmount(struct fs *fs);
int pfs_create_inode(struct pfs_fs *pfsp, struct disk_inode *din, int ino, struct inode **node_store);
int pfs_create_dirblock(struct pfs_fs *pfsp, int ino, char *name);
int pfs_rblock(struct pfs_fs *pfs, int blkno);
int pfs_wblock(struct pfs_fs *pfs, int blkno);
struct inode *pfs_get_root(struct fs *fs);
int pfs_load_dinode(struct pfs_fs *pfsp, int ino, struct inode **node_store);

const struct inode_ops *pfs_get_ops(int type);

#endif
