#include "vfs.h"
#include "pfs.h"
#include "dev.h"
#include "pmm.h"
#include "util/string.h"
#include "spike_interface/spike_utils.h"

//
// pfs_init: called by fs_init
//
void pfs_init(void) {
  int ret;
  if ((ret = pfs_mount("ramdisk0")) != 0)
    panic("failed: pfs: pfs_mount: %d.\n", ret);
}

int pfs_mount(const char *devname) {
  return vfs_mount(devname, pfs_do_mount);
}

/*
 * Mount VFS(struct fs)-pfs(struct pfs_fs)-RAM Device(struct device)
 *
 * ******** pfs MEM LAYOUT (112 BLOCKS) ****************
 *   super  |  inodes  |  bitmap  |  free blocks  *
 *     1 block   |    10    |     1    |     100       *
 * *****************************************************
 */
int pfs_do_mount(device *dev, fs **vfs_fs) {
  /*
   * 1. alloc fs structure
   * struct fs (vfs.h):
   *        union { struct pfs_fs pfs_info } fs_info: pfs_fs
   *        fs_type:  pfs (=0)
   *    function:
   *        fs_sync
   *        fs_get_root
   *        fs_unmount
   *        fs_cleanup
   */
  fs *fs = alloc_fs(PFS_TYPE); // set fs_type
  /*
   * 2. alloc pfs_fs structure
   * struct pfs_fs (pfs.h):
   *      super:    super
   *      dev:      the pointer to the device (struct device * in dev.h)
   *      freemap:  a block of bitmap (free: 0; used: 1)
   *      dirty:    true if super/freemap modified
   *      buffer:   buffer for non-block aligned io
   */
  pfs_fs *pfsp = fs_op_info(fs, PFS_TYPE);

  // 2.1. set [pfsp->dev] & [pfsp->dirty]
  pfsp->dev = dev;
  pfsp->dirty = 0;
  pfsp->buffer = alloc_page();

  // 2.3. usually read [super] (1 block) from device to pfsp->buffer
  //      BUT for volatile RAM Disk, there is no super remaining on disk

  //      build a new super
  pfsp->super.magic = PFS_MAGIC;
  pfsp->super.size = 1 + PFS_MAX_INODE_NUM + 1 + PFS_MAX_INODE_NUM * PFS_NDIRECT;
  pfsp->super.nblocks = PFS_MAX_INODE_NUM * PFS_NDIRECT; // only direct index blocks
  pfsp->super.ninodes = PFS_MAX_INODE_NUM;

  int ret;
  //      write the super to RAM Disk0
  memcpy(pfsp->buffer, &(pfsp->super), PFS_BLKSIZE);  // set io buffer
  if ((ret = pfs_wblock(pfsp, PFS_BLKN_SUPER)) != 0) // write to device
    panic("pfs: failed to write super!\n");

  // 2.4. similarly, build an empty [bitmap] and write to RAM Disk0
  pfsp->freemap = alloc_page();
  memset(pfsp->freemap, 0, PFS_BLKSIZE);
  pfsp->freemap[0] = 1; // the first data block is used for root directory

  //      write the bitmap to RAM Disk0
  memcpy(pfsp->buffer, pfsp->freemap, PFS_BLKSIZE);    // set io buffer
  if ((ret = pfs_wblock(pfsp, PFS_BLKN_BITMAP)) != 0) // write to device
    panic("pfs: failed to write bitmap!\n");

  // 2.5. build inodes (inode -> pfsp->buffer -> RAM Disk0)
  pfs_dinode *pinode = (pfs_dinode *)pfsp->buffer;
  pinode->size = 0;
  pinode->type = T_FREE;
  pinode->nlinks = 0;
  pinode->blocks = 0;

  //      write disk inodes to RAM Disk0
  for (int i = 1; i < pfsp->super.ninodes; ++i)
    if ((ret = pfs_wblock(pfsp, PFS_BLKN_INODE + i)) != 0)
      panic("pfs: failed to write inodes!\n");

  //      build root directory inode (ino = 0)
  pinode->size = sizeof(struct pfs_dir_entry);
  pinode->type = T_DIR;
  pinode->nlinks = 1;
  pinode->blocks = 1;
  pinode->direct[0] = PFS_BLKN_FREE;

  //      write root directory inode to RAM Disk0
  if ((ret = pfs_wblock(pfsp, PFS_BLKN_INODE)) != 0)
    panic("pfs: failed to write root inode!\n");

  // 2.6. write root directory block
  pfs_create_dirblock(pfsp, PFS_BLKN_INODE, "/");

  //      write root directory block to RAM Disk0
  if ((ret = pfs_wblock(pfsp, PFS_BLKN_FREE)) != 0)
    panic("pfs: failed to write root inode!\n");

  // 3. mount functions
  fs->fs_get_root = pfs_get_root;
  fs->fs_unmount = pfs_unmount;
  fs->fs_cleanup = pfs_cleanup;

  *vfs_fs = fs;

  return 0;
}

int pfs_rblock(struct pfs_fs *pfs, int blkno) {
  return dev_op_io(pfs->dev, blkno,pfs->buffer, 0);
}

int pfs_wblock(struct pfs_fs *pfs, int blkno) {
  return dev_op_io(pfs->dev, blkno,pfs->buffer, 1);
}

//
// Return root inode of filesystem.
//
inode *pfs_get_root(struct fs *fs) {
  inode *node;
  // get pfs pointer
  pfs_fs *pfsp = fs_op_info(fs, PFS_TYPE);
  // load the root inode
  pfs_load_dinode(pfsp, PFS_BLKN_INODE, &node);
  return node;
}

//
// load inode from disk (ino: inode number)
//
int pfs_load_dinode(pfs_fs *pfsp, int ino, inode **node_store) {
  inode *node;
  int ret;
  // load inode[ino] from disk -> pfs buffer -> dnode
  if ((ret = pfs_rblock(pfsp, PFS_BLKN_INODE)) != 0)
    panic("pfs: failed to read inode!\n");
  pfs_dinode *dnode = (pfs_dinode *)pfsp->buffer;

  // build inode according to dinode
  if ((ret = pfs_create_inode(pfsp, dnode, ino, &node)) != 0)
    panic("pfs: failed to create inode from dinode!\n");
  *node_store = node;
  return 0;
}

/*
 * Create an inode in kernel according to din
 *
 */
int pfs_create_inode(struct pfs_fs *pfsp, struct pfs_dinode *din, int ino, struct inode **node_store) {
  // 1. alloc an vfs-inode in memory
  struct inode *node = alloc_inode(PFS_TYPE);
  // 2. copy disk-inode data
  struct pfs_dinode *dnode = vop_info(node, PFS_TYPE);
  dnode->size = din->size;
  dnode->type = din->type;
  dnode->nlinks = din->nlinks;
  dnode->blocks = din->blocks;
  for (int i = 0; i < PFS_NDIRECT; ++i)
    dnode->direct[i] = din->direct[i];

  // 3. set inum, ref_count, in_fs, in_ops
  node->inum = ino;
  node->ref_count = 0;
  node->in_fs = (struct fs *)pfsp;
  node->in_ops = pfs_get_ops(dnode->type);

  *node_store = node;
  return 0;
}

//
// create a directory block
//
int pfs_create_dirblock(struct pfs_fs *pfsp, int ino, char *name) {
  pfs_dir_entry *de = (pfs_dir_entry *)pfsp->buffer;
  de->inum = ino;
  strcpy(de->name, name);
  return 0;
}

int pfs_unmount(struct fs *fs) {
  return 0;
}

void pfs_cleanup(struct fs *fs) {
  return;
}

int pfs_opendir(struct inode *node, int open_flags) {
  int fd = 0;
  return fd;
}

int pfs_openfile(struct inode *node, int open_flags) {
  return 0;
}

int pfs_close(struct inode *node) {
  int fd = 0;
  return fd;
}

int pfs_fstat(struct inode *node, struct fstat *stat) {
  pfs_dinode *dnode = vop_info(node, PFS_TYPE);
  stat->st_mode = dnode->type;
  stat->st_nlinks = dnode->nlinks;
  stat->st_blocks = dnode->blocks;
  stat->st_size = dnode->size;
  return 0;
}

/*
 * lookup the path in the given directory
 * @param node: directory node
 * @param path: filename
 * @param node_store: store the file inode
 * @return
 *    0: the file is found
 *    1: the file is not found, need to be created
 */
int pfs_lookup(struct inode *node, char *path, struct inode **node_store) {
  struct pfs_dinode *dnode = vop_info(node, PFS_TYPE);
  struct pfs_fs *pfsp = fs_op_info(node->in_fs, PFS_TYPE);

  // 读入一个dir block，遍历direntry，查找filename
  struct pfs_dir_entry *de = NULL;

  int nde = dnode->size / sizeof(struct pfs_dir_entry);
  int maxde = PFS_BLKSIZE / sizeof(struct pfs_dir_entry);

  for (int i = 0; i < nde; ++i) {
    if (i % maxde == 0) {                         // 需要切换block基地址
      pfs_rblock(pfsp, dnode->direct[i / maxde]); // 读入第i/maxde个block
      de = (struct pfs_dir_entry *)pfsp->buffer;
    }
    if (strcmp(de->name, path) == 0) { // 找到文件
      pfs_rblock(pfsp, de->inum);     // 读入文件的dinode块到buffer
      pfs_create_inode(pfsp, (struct pfs_dinode *)pfsp->buffer, de->inum, node_store);
      return 0;
    }
    ++de;
  }
  return 1; // no file inode, need to be created
}

int pfs_alloc_block(struct pfs_fs *pfs) {
  int freeblkno = -1;
  for (int i = 0; i < pfs->super.nblocks; ++i) {
    if (pfs->freemap[i] == 0) { // find a free block
      pfs->freemap[i] = 1;
      freeblkno = PFS_BLKN_FREE + i;
      // sprint("pfs_alloc_block: blkno: %d\n", freeblkno);
      break;
    }
  }
  if (freeblkno == -1)
    panic("pfs_alloc_block: no free block!\n");
  return freeblkno;
}

/*
 * create a file named "name" in the given directory (node)
 * @param node: dir node
 * @param name: file name
 * @param node_store: store the file inode
 */
int pfs_create(inode *dir, const char *name, inode **node_store) {
  // 1. build dinode, inode for a new file
  // alloc a disk-inode on disk
  fs *fs = dir->in_fs;
  pfs_fs *pfs = fs_op_info(fs, PFS_TYPE);
  
  pfs_dinode *din;
  int blkno = 0; // inode blkno = ino
  for (int i = 0; i < PFS_MAX_INODE_NUM; ++i) {
    blkno = PFS_BLKN_INODE + i;
    pfs_rblock(pfs, blkno); // read dinode block
    din = (pfs_dinode *)pfs->buffer;
    if (din->type == T_FREE) // find a free inode block
      break;
  }
  int ino = blkno;

  // build a dinode for the file
  din->size = 0;
  din->type = T_FILE;
  din->nlinks = 1;
  din->blocks = 1;
  din->direct[0] = pfs_alloc_block(pfs); // find a free block

  // write the dinode to disk
  pfs_wblock(pfs, blkno);

  // build inode according to dinode
  int ret;
  if ((ret = pfs_create_inode(pfs, din, ino, node_store)) != 0)
    panic("pfs_create: failed to create inode from dinode!\n");

  // struct inode * rino = *node_store;
  // struct pfs_dinode * pfs_din = vop_info(rino, PFS_TYPE);

  // 2. add a directory entry in the dir file
  // 2.1. 修改dir-inode-block on disk
  pfs_rblock(pfs, dir->inum);
  din = (struct pfs_dinode *)pfs->buffer;
  din->size += sizeof(struct pfs_dir_entry);
  pfs_wblock(pfs, dir->inum);

  // 2.2. 修改dir-data-block
  din = vop_info(dir, PFS_TYPE);
  // din->size += sizeof(struct pfs_dir_entry);

  // 接在din->size后面
  blkno = din->direct[din->size / PFS_BLKSIZE];

  // 读入数据块
  pfs_rblock(pfs, blkno);

  // 添加内容(ino, name)
  uint64 addr = (uint64)pfs->buffer + din->size % PFS_BLKSIZE;
  struct pfs_dir_entry *de = (struct pfs_dir_entry *)addr;
  de->inum = ino;
  strcpy(de->name, name);

  // 写回设备
  pfs_wblock(pfs, blkno);

  return 0;
}

int pfs_read(inode *node, char *buf, uint64 len) {
  struct pfs_dinode *din = vop_info(node, PFS_TYPE);
  struct fs *fs = node->in_fs;
  struct pfs_fs *pfs = fs_op_info(fs, PFS_TYPE);

  len = MIN(len, din->size);
  sprint("pfs_read: len: %d\n", len);
  char buffer[len + 1];

  int readtime = ROUNDUP(len, PFS_BLKSIZE) / PFS_BLKSIZE;
  sprint("pfs_read: read %d times from disk. \n", readtime);

  int offset = 0, i = 0;

  while (offset + PFS_BLKSIZE < len) {
    pfs_rblock(pfs, din->direct[i]);
    memcpy(buffer + offset, pfs->buffer, PFS_BLKSIZE);
    offset += PFS_BLKSIZE;
    i++;
  }
  pfs_rblock(pfs, din->direct[i]);
  memcpy(buffer + offset, pfs->buffer, len - offset);
  buffer[len] = '\0';
  strcpy(buf, buffer);
  return 0;
}

int pfs_write(struct inode *node, char *buf, uint64 len) {
  // write inode
  struct pfs_dinode *din = vop_info(node, PFS_TYPE);
  din->size = (strlen(buf) + 1) * sizeof(char);

  struct fs *fs = node->in_fs;
  struct pfs_fs *pfs = fs_op_info(fs, PFS_TYPE);

  // write data
  int writetime = len / PFS_BLKSIZE;
  int remain = len % PFS_BLKSIZE;
  if (remain > 0)
    ++writetime;
  sprint("pfs_write: write %d times from disk. \n", writetime);

  int offset = 0;
  int i = 0;
  for (; i < writetime; ++i) {
    // find a block
    if (i == din->blocks) {
      din->direct[i] = pfs_alloc_block(pfs);
      ++din->blocks;
    }
    if (i == writetime - 1)
      memcpy(pfs->buffer, buf + offset, PFS_BLKSIZE);
    else
      memcpy(pfs->buffer, buf + offset, remain);
    pfs_wblock(pfs, din->direct[i]);
    offset += PFS_BLKSIZE;
  }

  int nblocks = din->blocks;
  // write disk-inode (size, blocks)
  pfs_rblock(pfs, node->inum);
  din = (struct pfs_dinode *)pfs->buffer; // dinode
  din->size = (strlen(buf) + 1) * sizeof(char);
  din->blocks = nblocks;
  pfs_wblock(pfs, node->inum);
  return 0;
}

// pfs DIR operations correspond to the abstract operations on a inode.
static const struct inode_ops pfs_node_dirops = {
    .vop_open = pfs_opendir,
    .vop_close = pfs_close,
    .vop_fstat = pfs_fstat,
    .vop_lookup = pfs_lookup,
    .vop_create = pfs_create,
};

// pfs FILE operations correspond to the abstract operations on a inode.
static const struct inode_ops pfs_node_fileops = {
    .vop_open = pfs_openfile,
    .vop_close = pfs_close,
    .vop_read = pfs_read,
    .vop_write = pfs_write,
    .vop_fstat = pfs_fstat,
};

const struct inode_ops *pfs_get_ops(int type) {
  switch (type) {
  case T_DIR:
    return &pfs_node_dirops;
  case T_FILE:
    return &pfs_node_fileops;
  default:
    panic("invalid pfs option type\n");
  }
}