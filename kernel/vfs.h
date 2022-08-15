#ifndef _VFS_H_
#define _VFS_H_

#include "dev.h"
#include "file.h"
#include "pfs.h"
#include "util/types.h"

// inode type
#define T_FREE 0
#define T_DEV 0x1
#define T_DIR 0x2
#define T_FILE 0x3

#define MAX_DEV 8     // the maximum number of vfs_dev_list
#define MAX_DEVNAME 64 // the maximum length of devname

// // inode flags
// #define I_BUSY 0x1
// #define I_VALID 0x2

/*
 * Reference: uCore
 * Abstract filesystem. (Or device accessible as a file.)
 *
 * Information:
 *      fs_info   : filesystem-specific data (pfs_fs)
 *      fs_type   : filesystem type
 * Operations:
 *      fs_sync       - Flush all dirty buffers to disk.
 *      fs_get_root   - Return root inode of filesystem.
 *      fs_unmount    - Attempt unmount of filesystem.
 *      fs_cleanup    - Cleanup of filesystem.
 *
 * fs_get_root should increment the refcount of the inode returned.
 * It should not ever return NULL.
 *
 * If fs_unmount returns an error, the filesystem stays mounted, and
 * consequently the struct fs instance should remain valid. On success,
 * however, the filesystem object and all storage associated with the
 * filesystem should have been discarded/released.
 *
 */
/*
 * VFS 层的文件系统抽象
 * struct fs:
 *        union { struct pfs_fs pfs_info } fs_info: 给具体文件系统实现的结构体空间
 *        fs_type: 确定使用的是哪个具体文件系统 (PFS_TYPE)
 *    function:
 *        fs_sync:
 *        fs_get_root: 指向具体文件系统实现访问根inode的函数
 *        fs_unmount:
 *        fs_cleanup:
 */
typedef struct fs {
  union {
    struct pfs_fs __PFS_TYPE_info;
  } fs_info;   // filesystem-specific data
  int fs_type; // filesystem type

  struct inode *(*fs_get_root)(struct fs *fs); // Return root inode of filesystem.
  int (*fs_unmount)(struct fs *fs);            // Attempt unmount of filesystem.
  void (*fs_cleanup)(struct fs *fs);           // Cleanup of filesystem.
} fs;

// virtual file system interfaces
#define fs_op_info(fs, type) &(fs->fs_info.__##type##_info)
#define fs_op_get_root(fs) ((fs)->fs_get_root(fs))

typedef struct inode {
  union {
    device __device_info;
    pfs_dinode __PFS_TYPE_inode_info;
  } in_info;
  int in_type;
  int inum;                       // inode number on-disk
  int ref_count;                  // reference count
  struct fs *in_fs;               // file system
  const struct inode_ops *in_ops; // inode options
} inode;

// virtual file system inode interfaces
#define vop_info(inode, type) &(inode->in_info.__##type##_inode_info)

#define vop_fstat(node, stat) (node->in_ops->vop_fstat(node, stat))
#define vop_lookup(node, path, node_store) (node->in_ops->vop_lookup(node, path, node_store))
#define vop_create(node, name, node_store) (node->in_ops->vop_create(node, name, node_store))
#define vop_read(node, buf, len) (node->in_ops->vop_read(node, buf, len))
#define vop_write(node, buf, len) (node->in_ops->vop_write(node, buf, len))

typedef struct inode_ops {
  int (*vop_open)(struct inode *node, int open_flags);
  int (*vop_close)(struct inode *node);
  int (*vop_read)(struct inode *node, char *buf, uint64 len);
  int (*vop_write)(struct inode *node, char *buf, uint64 len);
  int (*vop_fstat)(struct inode *node, struct fstat *stat);
  // int (*vop_fsync)(struct inode *node);
  // int (*vop_namefile)(struct inode *node, struct iobuf *iob);
  // int (*vop_getdirentry)(struct inode *node, struct iobuf *iob);
  // int (*vop_reclaim)(struct inode *node);
  // int (*vop_gettype)(struct inode *node, int *type_store);
  // int (*vop_tryseek)(struct inode *node, off_t pos);
  // int (*vop_truncate)(struct inode *node, off_t len);
  int (*vop_create)(struct inode *node, const char *name, struct inode **node_store);
  int (*vop_lookup)(struct inode *node, char *path, struct inode **node_store);
  // int (*vop_ioctl)(struct inode *node, int op, void *data);
} inode_ops;

//
// device info entry in vdev_list
//
typedef struct vfs_dev_t {
  const char *devname; // the name of the device
  struct device *dev;  // the pointer to the device (dev.h)
  struct fs *fs;       // the file system mounted to the device
} vfs_dev_t;

extern vfs_dev_t *vdev_list[MAX_DEV]; // device info list in vfs layer

/*
 * Virtual File System layer functions.
 *
 * The VFS layer translates operations on abstract on-disk files or
 * pathnames to operations on specific files on specific filesystems.
 */
// void vfs_init(void);
// void vfs_cleanup(void);

fs *alloc_fs(int fs_type);
inode *alloc_inode(int in_type);

int vfs_mount(const char *devname, int (*mountfunc)(device *dev, fs **vfs_fs));

int vfs_get_root(const char *devname, inode **root_store);

int vfs_open(char *path, int flags, inode **inode_store);
int vfs_close(inode *node);
// int vfs_link(char *old_path, char *new_path);
// int vfs_symlink(char *old_path, char *new_path);
// int vfs_readlink(char *path, struct iobuf *iob);
// int vfs_mkdir(char *path);
// int vfs_unlink(char *path);
// int vfs_rename(char *old_path, char *new_path);
// int vfs_chdir(char *path);
// int vfs_getcwd(struct iobuf *iob);

/*
 * VFS layer mid-level operations.
 *
 *    vfs_lookup     - Like VOP_LOOKUP, but takes a full device:path name,
 *                     or a name relative to the current directory, and
 *                     goes to the correct filesystem.
 *    vfs_lookparent - Likewise, for VOP_LOOKPARENT.
 *
 * Both of these may destroy the path passed in.
 */
int vfs_lookup(char *path, inode **node_store);
int vfs_lookup_parent(char *path, inode **node_store, char **endp);
int get_device(char *path, char **subpath, inode **node_store);

#endif