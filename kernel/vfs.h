//vitual file system
#ifndef _VFS_H_
#define _VFS_H_

#include "dev.h"
#include "filesystem.h"
#include "pfs.h"
#include "util/types.h"

// inode type
#define T_FREE 0
#define T_DEV 0x1
#define T_DIR 0x2
#define T_FILE 0x3

#define MAX_DEV 8     // the maximum number of vfs_dev_list
#define MAX_DEVNAME 64 // the maximum length of devname


//
// virtual file system struct
//
typedef struct fs {
  union {
    struct pfs_fs __PFS_TYPE_info;
  } fs_info;   // filesystem-specific data define in pfs.h
  int fs_type; // filesystem type

  struct inode *(*fs_get_root)(struct fs *fs); // return root inode of filesystem.
  int (*fs_unmount)(struct fs *fs);            // attempt unmount of filesystem.
  
} fs;

// virtual file system interfaces
#define fs_op_info(fs, type) &(fs->fs_info.__##type##_info)
#define fs_op_get_root(fs) ((fs)->fs_get_root(fs))

fs *alloc_fs(int fs_type);

typedef struct inode {
  union {
    device __device_info;
    disk_inode __PFS_TYPE_inode_info;
  } in_info;
  int in_type;                    //inode type T_FREE T_DEV T_DIR T_FILE
  int inum;                       // inode number on-disk
  int ref_count;                  // reference count
  struct fs *in_fs;               // file system
  const struct inode_ops *in_ops; // inode options
} inode;

typedef struct inode_ops {
  int (*vop_open)(struct inode *node, int open_flags);
  int (*vop_close)(struct inode *node);
  int (*vop_read)(struct inode *node, char *buf);
  int (*vop_write)(struct inode *node, char *buf);
  int (*vop_fstat)(struct inode *node, struct fstat *stat);
  int (*vop_create)(struct inode *node, const char *name, struct inode **node_store);
  int (*vop_lookup)(struct inode *node, char *path, struct inode **node_store);
} inode_ops;

// virtual file system inode interfaces
#define vop_info(inode, type) &(inode->in_info.__##type##_inode_info)
#define vop_read(inode, buf) (inode->in_ops->vop_read(inode, buf))
#define vop_write(inode, buf) (inode->in_ops->vop_write(inode, buf))
#define vop_fstat(inode, stat) (inode->in_ops->vop_fstat(inode, stat))
#define vop_create(inode, name, node_store) (inode->in_ops->vop_create(inode, name, node_store))
#define vop_lookup(inode, path, node_store) (inode->in_ops->vop_lookup(inode, path, node_store))

int vfs_open(char *path, int flags, inode **inode_store);
int vfs_close(inode *node);
inode *alloc_inode(int in_type);

//
// device info entry in vdev_list
//
typedef struct vfs_dev_t {
  const char *devname; // the name of the device
  struct device *dev;  // the pointer to the device (dev.h)
  struct fs *fs;       // the file system mounted to the device
} vfs_dev_t;

extern vfs_dev_t *vdev_list[MAX_DEV]; // device info list in vfs layer

int vfs_mount(const char *devname, int (*mountfunc)(device *dev, fs **vfs_fs));
int vfs_get_root(const char *devname, inode **root_store);
int vfs_lookup(char *path, inode **node_store);
int vfs_lookup_parent(char *path, inode **node_store, char **endp);
int get_device(char *path, char **subpath, inode **node_store);

#endif