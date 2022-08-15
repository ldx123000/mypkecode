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
  if ((ret = pfs_mount("Disk_D")) != 0)
    panic("failed: pfs: pfs_mount: %d.\n", ret);
  if ((ret = pfs_mount("Disk_1")) != 0)
    panic("failed: pfs: pfs_mount: %d.\n", ret);
}

int pfs_mount(const char *devname) {
  return vfs_mount(devname, pfs_do_mount);
}

//
// Mount pfs struct
//
int pfs_do_mount(device *dev, fs **vfs_fs) {

  // alloc fs in memory
  fs *fs = alloc_fs(PFS_TYPE); // set fs_type

  // set pfs_fs in fs structure
  pfs_fs *pfsp = fs_op_info(fs, PFS_TYPE);
  pfsp->dev = dev;
  pfsp->dirty = 0;
  pfsp->buffer = alloc_page();

  //      build a new super and save it's info to disk
  pfsp->super.magic = PFS_MAGIC;
  pfsp->super.size = 1 + 1 + PFS_MAX_DIN_NUM + PFS_MAX_DIN_NUM * PFS_NDIRECT;
  pfsp->super.nblocks = PFS_MAX_DIN_NUM * PFS_NDIRECT; // only direct index blocks
  pfsp->super.ninodes = PFS_MAX_DIN_NUM;

  int ret;
  // write the super block to disk
  memcpy(pfsp->buffer, &(pfsp->super), PFS_BLKSIZE); // set io buffer
  if (pfs_wblock(pfsp, PFS_BLKN_SUPER) != 0)         // write to device
    panic("pfs: failed to write super!\n");

  // build a new bitmap and save it's info to disk
  pfsp->freemap = alloc_page();
  memset(pfsp->freemap, 0, PFS_BLKSIZE);
  pfsp->freemap[0] = 1;                               // the first data block is used for root directory
  memcpy(pfsp->buffer, pfsp->freemap, PFS_BLKSIZE);   // set io buffer
  if ((ret = pfs_wblock(pfsp, PFS_BLKN_BITMAP)) != 0) // write to device
    panic("pfs: failed to write bitmap!\n");

  // build new disk_inodes and save it's info to disk
  disk_inode *din = (disk_inode *)pfsp->buffer;

  // build new root directory disk_inode
  din->size = sizeof(pfs_dir_entry);
  din->type = T_DIR;
  din->nlinks = 1;
  din->blocks = 1;
  din->direct[0] = PFS_BLKN_FREE;
  if ((ret = pfs_wblock(pfsp, PFS_BLKN_DIN)) != 0) //      write to disk
    panic("pfs: failed to write root inode!\n");

  // build other disk_inodes
  din->size = 0;
  din->type = T_FREE;
  din->nlinks = 0;
  din->blocks = 0;
  for (int i = 1; i < pfsp->super.ninodes; i++) //      write to disk
    if ((ret = pfs_wblock(pfsp, PFS_BLKN_DIN + i)) != 0)
      panic("pfs: failed to write inodes!\n");

  // build root directory block
  pfs_create_dirblock(pfsp, PFS_BLKN_DIN, "/");
  if ((ret = pfs_wblock(pfsp, PFS_BLKN_FREE)) != 0) //      write to disk
    panic("pfs: failed to write root inode!\n");

  // mount functions
  fs->fs_get_root = pfs_get_root;
  fs->fs_unmount = pfs_unmount;

  *vfs_fs = fs;

  return 0;
}

//
// build inode according to disk_inode
//
int pfs_create_inode(pfs_fs *pfsp, disk_inode *din, int ino, inode **node_store) {
  // alloc inode in memory
  inode *node = alloc_inode(PFS_TYPE);

  // get disk_inode info from inode and set it's info by copying other disk-inode data
  disk_inode *din_from_node = vop_info(node, PFS_TYPE);
  din_from_node->size = din->size;
  din_from_node->type = din->type;
  din_from_node->nlinks = din->nlinks;
  din_from_node->blocks = din->blocks;
  for (int i = 0; i < PFS_NDIRECT; ++i)
    din_from_node->direct[i] = din->direct[i];

  // set node's info
  node->inum = ino;
  node->ref_count = 0;
  node->in_fs = (struct fs *)pfsp;
  node->in_ops = pfs_get_ops(din_from_node->type);

  // store this node
  *node_store = node;
  return 0;
}

//
// create a directory block
//
int pfs_create_dirblock(struct pfs_fs *pfsp, int ino, char *name) {
  pfs_dir_entry *dir_e = (pfs_dir_entry *)pfsp->buffer; // for pfs_fs io to disk
  dir_e->inum = ino;
  strcpy(dir_e->name, name);
  return 0;
}

int pfs_alloc_block(struct pfs_fs *pfs) {
  int freeblkno = -1;
  for (int i = 0; i < pfs->super.nblocks; ++i) {
    if (pfs->freemap[i] == 0) { // find a free block to use
      pfs->freemap[i] = 1;
      freeblkno = PFS_BLKN_FREE + i;
      break;
    }
  }
  if (freeblkno == -1)
    panic("no free block now!\n");
  return freeblkno;
}

//
// create a file with name in the given directory
//
int pfs_create(inode *dir, const char *name, inode **node_store) {
  fs *fs = dir->in_fs;
  pfs_fs *pfs = fs_op_info(fs, PFS_TYPE);

  // alloc a disk-inode on disk
  disk_inode *din;
  int blkno;
  for (blkno = PFS_BLKN_DIN; blkno < PFS_MAX_DIN_NUM + PFS_BLKN_DIN; blkno++) {
    pfs_rblock(pfs, blkno); // read dinode block
    din = (disk_inode *)pfs->buffer;
    if (din->type == T_FREE) // find a free inode block
      break;
  }
  int ino = blkno; // inode blkno

  // set disk_inode info
  din->size = 0;
  din->type = T_FILE;
  din->nlinks = 1;
  din->blocks = 1;
  din->direct[0] = pfs_alloc_block(pfs); // find a free block

  // write the disk_inode data to disk
  pfs_wblock(pfs, blkno);
  if (pfs_create_inode(pfs, din, ino, node_store) != 0) // build inode according to dinode
    panic("pfs_create: failed to create inode from dinode!\n");

  // add a directory entry in the dir file

  // change dir-inode's block on disk
  pfs_rblock(pfs, dir->inum);
  din = (disk_inode *)pfs->buffer;
  din->size += sizeof(pfs_dir_entry);
  pfs_wblock(pfs, dir->inum);

  // get din to add info
  din = vop_info(dir, PFS_TYPE);
  blkno = din->direct[din->size / PFS_BLKSIZE];
  pfs_rblock(pfs, blkno);

  // add ino,name
  uint64 addr = (uint64)pfs->buffer + din->size % PFS_BLKSIZE;
  pfs_dir_entry *dir_e = (struct pfs_dir_entry *)addr;
  dir_e->inum = ino;
  strcpy(dir_e->name, name);

  // write to disk
  pfs_wblock(pfs, blkno);

  return 0;
}

int pfs_rblock(struct pfs_fs *pfs, int blkno) {
  return dev_op_io(pfs->dev, blkno, pfs->buffer, 0);
}

int pfs_wblock(struct pfs_fs *pfs, int blkno) {
  return dev_op_io(pfs->dev, blkno, pfs->buffer, 1);
}

//
// read data by inode
//
int pfs_read(inode *node, char *buf) {
  struct disk_inode *din = vop_info(node, PFS_TYPE);
  struct fs *fs = node->in_fs;
  struct pfs_fs *pfs = fs_op_info(fs, PFS_TYPE);

  uint64 len=strlen(buf);
  len = MIN(len, din->size);
  char bio[len + 1];

  int offset = 0, i = 0;

  while (offset + PFS_BLKSIZE < len) {
    pfs_rblock(pfs, din->direct[i]);
    memcpy(bio + offset, pfs->buffer, PFS_BLKSIZE);
    offset += PFS_BLKSIZE;
    i++;
  }
  pfs_rblock(pfs, din->direct[i]);
  memcpy(bio + offset, pfs->buffer, len - offset);
  bio[len] = '\0';
  strcpy(buf, bio);
  return 0;
}

//
// write data by inode
//
int pfs_write(struct inode *node, char *buf) {
  struct disk_inode *din = vop_info(node, PFS_TYPE);
  struct fs *fs = node->in_fs;
  struct pfs_fs *pfs = fs_op_info(fs, PFS_TYPE);

  din->size = (strlen(buf) + 1) * sizeof(char);
  uint64 len=strlen(buf);
  
  // write data
  int total = len / PFS_BLKSIZE;
  int remain = len % PFS_BLKSIZE;
  if (remain != 0)
    total++;

  for (int offset = 0, i = 0; i < total; ++i) {
    // find a block
    if (i == din->blocks) {
      din->direct[i] = pfs_alloc_block(pfs);
      ++din->blocks;
    }
    if (i == total - 1)
      memcpy(pfs->buffer, buf + offset, PFS_BLKSIZE);
    else
      memcpy(pfs->buffer, buf + offset, remain);
    pfs_wblock(pfs, din->direct[i]);
    offset += PFS_BLKSIZE;
  }

  // change disk inode info
  pfs_rblock(pfs, node->inum);
  int tmp = din->blocks;
  din = (disk_inode *)pfs->buffer; // dinode
  din->size = (strlen(buf) + 1) * sizeof(char);
  din->blocks = tmp;
  pfs_wblock(pfs, node->inum);
  return 0;
}

//
// return root inode of filesystem.
//
inode *pfs_get_root(struct fs *fs) {
  inode *node;
  // get pfs pointer
  pfs_fs *pfsp = fs_op_info(fs, PFS_TYPE);
  // load the root inode
  pfs_load_dinode(pfsp, PFS_BLKN_DIN, &node);
  return node;
}

//
// load inode from disk
//
int pfs_load_dinode(pfs_fs *pfsp, int ino, inode **node_store) {
  inode *node;
  // load disk_inode from disk
  if (pfs_rblock(pfsp, PFS_BLKN_DIN) != 0)
    panic("fail to read inode!\n");

  // disk_inode get data
  disk_inode *din = (disk_inode *)pfsp->buffer;

  // build inode according to disk_inode
  if ((pfs_create_inode(pfsp, din, ino, &node)) != 0)
    panic("fail to create inode!\n");

  // get inode
  *node_store = node;
  return 0;
}

int pfs_unmount(struct fs *fs) {
  return 0;
}

int pfs_fstat(struct inode *node, struct fstat *stat) {
  disk_inode *din = vop_info(node, PFS_TYPE);
  stat->st_mode = din->type;
  stat->st_nlinks = din->nlinks;
  stat->st_blocks = din->blocks;
  stat->st_size = din->size;
  return 0;
}

//
// lookup the path in the directory
// if fail to found file, create a new file here
//
int pfs_lookup(inode *node, char *path, inode **node_store) {
  // get disk_inode,pfs_fs info from inode
  disk_inode *din = vop_info(node, PFS_TYPE);
  pfs_fs *pfsp = fs_op_info(node->in_fs, PFS_TYPE);

  // read a directory block, direntry 查找filename
  pfs_dir_entry *dir_e = NULL;

  int total = din->size / sizeof(pfs_dir_entry);
  int BLKlen = PFS_BLKSIZE / sizeof(pfs_dir_entry);
  // traversal direntry
  for (int i = 0; i < total; ++i) {
    if (i % BLKlen == 0) { // need change to next block dir_entry
      int num = i / BLKlen;
      pfs_rblock(pfsp, din->direct[num]);    // read another block
      dir_e = (pfs_dir_entry *)pfsp->buffer; // get dir_e data
    }
    if (strcmp(dir_e->name, path) == 0) {                   // find filename
      pfs_rblock(pfsp, dir_e->inum);                        // read disk_inode data from disk to buffer
      disk_inode *din = pfsp->buffer;                       // get disk_inode
      pfs_create_inode(pfsp, din, dir_e->inum, node_store); // build inode according to disk_inode
      return 0;
    }
    dir_e++; // change to next dir_entry
  }
  return 1; // fail to find file inode, means create a new file here
}

// pfs DIR operations correspond to the abstract operations on a inode.
static const struct inode_ops pfs_node_dirops = {
    .vop_fstat = pfs_fstat,
    .vop_lookup = pfs_lookup,
    .vop_create = pfs_create,
};

// pfs FILE operations correspond to the abstract operations on a inode.
static const struct inode_ops pfs_node_fileops = {
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
    panic("invalid pfs_option type!\n");
  }
}