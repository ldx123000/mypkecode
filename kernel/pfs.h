#ifndef _pfs_H_
#define _pfs_H_

#include "vfs.h"
#include "riscv.h"
#include "util/functions.h"
#include "util/types.h"

#define PFS_TYPE          0
#define PFS_MAGIC         0x2f8dbe2a
#define PFS_BLKSIZE       PGSIZE
#define PFS_MAX_INODE_NUM 10
#define PFS_MAX_FNAME_LEN 28
#define PFS_NDIRECT       10

// pfs block number
#define PFS_BLKN_SUPER    0
#define PFS_BLKN_INODE    1
#define PFS_BLKN_BITMAP   11
#define PFS_BLKN_FREE     12

struct fs;
struct inode;
struct file;

// file system super block
typedef struct super {
  int magic;         // magic number of the 
  int size;          // Size of file system image (blocks)
  int nblocks;       // Number of data blocks
  int ninodes;       // Number of inodes.
}super;

// filesystem for pfs
typedef struct pfs_fs {
  struct super super;  // pfs_superblock
  struct device * dev;          // device mounted on
  int * freemap;                // blocks in use are marked 1
  bool dirty;                   // true if super/freemap modified
  void * buffer;                // buffer for io
}pfs_fs;

// inode on disk
typedef struct pfs_dinode{
  int size;               // size of the file (in bytes)
  int type;               // one of T_FREE, T_DEV, T_FILE, T_DIR
  int nlinks;             // number of hard links
  int blocks;             // number of blocks is using
  uint64 direct[PFS_NDIRECT]; // direct blocks
}pfs_dinode;

// directory entry
 typedef struct pfs_dir_entry {
  int inum;                     // inode number
  char name[PFS_MAX_FNAME_LEN]; // file name
}pfs_dir_entry;

void pfs_init(void);
int pfs_mount(const char *devname);
int pfs_do_mount(struct device *dev, struct fs **vfs_fs);

int pfs_rblock(struct pfs_fs *pfs, int blkno);
int pfs_wblock(struct pfs_fs *pfs, int blkno);

struct inode * pfs_get_root(struct fs *fs);
int pfs_unmount(struct fs *fs);
void pfs_cleanup(struct fs *fs);

// int pfs_rblock(struct pfs_fs *pfs, void *buf, int32 blkno, int32 nblks);
// int pfs_wblock(struct pfs_fs *pfs, void *buf, int32 blkno, int32 nblks);
// int pfs_rbuf(struct pfs_fs *pfs, void *buf, size_t len, int32 blkno, off_t offset);
// int pfs_wbuf(struct pfs_fs *pfs, void *buf, size_t len, int32 blkno, off_t offset);
// int pfs_sync_super(struct pfs_fs *pfs);
// int pfs_sync_freemap(struct pfs_fs *pfs);
// int pfs_clear_block(struct pfs_fs *pfs, int32 blkno, int32 nblks);

int pfs_load_dinode(struct pfs_fs *pfsp, int ino, struct inode **node_store);
int pfs_create_inode(struct pfs_fs *pfsp, struct pfs_dinode * din, int ino, struct inode **node_store);
int pfs_create_dirblock(struct pfs_fs *pfsp, int ino, char * name);

const struct inode_ops * pfs_get_ops(int type);

#endif
