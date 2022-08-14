#include "vfs.h"
#include "pmm.h"
#include "spike_interface/spike_utils.h"
#include "util/string.h"

#include "hostfs.h"

struct vfs_dev_t *vdev_list[MAX_DEV]; // device info list in vfs layer

//
// alloc an abstract file system
//
fs *alloc_fs(int fs_type) {
  fs *filesystem = (struct fs *)alloc_page();
  filesystem->fs_type = fs_type;
  return filesystem;
}

//
// alloc an inode
//
inode *alloc_inode(int in_type) {
  inode *node = (inode *)alloc_page();
  node->in_type = in_type;
  return node;
}

//
// mount a file system to the device
//
int vfs_mount(const char *devname, int (*mountfunc)(struct device *dev, struct fs **vfs_fs)) {
  vfs_dev_t *devp = NULL;
  // find an entry from device_list
  for (int i = 0; i < MAX_DEV; i++) {
    if (strcmp(vdev_list[i]->devname, devname) == 0) {
      devp = vdev_list[i];
      break;
    }
  }
  if (devp == NULL)
    panic("vfs_mount: Cannot find the device entry!\n");

  // mount the specific file system to the device with mountfunc
  if (mountfunc(devp->dev, &(devp->fs)) == 0)
    sprint("VFS: file system successfully mounted to %s\n", devp->devname);
  return 0;
}

//
// vfs_open
//
int vfs_open(char *path, int open_flags, struct inode **inode_store) {
  // get open_flags
  // int writable = 0;
  int creatable = open_flags & O_CREATE;
  // switch (open_flags & MASK_FILEMODE) { // 获取flags的低2位
  // case O_RDONLY:
  //   break;
  // case O_WRONLY:
  // case O_RDWR:
  //   writable = 1;
  //   break;
  // default:
  //   panic("fs_open: invalid open flags!\n");
  // }

  // lookup the path, and create an related inode
  struct inode *node;
  int ret = vfs_lookup(path, &node);

  // the path belongs to the host device
  if (ret == -1) {
    int kfd = host_open(path, open_flags);
    return kfd;
  }
  // Case 2.1: if the path belongs to the PKE device and the file already exists
  //    continue
  // Case 2.2: if the file doesn't exists
  if (ret == 1) {
    // Case 2.2.1: the file cannot be created, exit
    if (creatable == 0)
      panic("vfs_open: open a non-existent-uncreatable file!\n");

    // Case 2.2.2: create a file, and get its inode
    char *filename;    // file name
    struct inode *dir; // dir inode
    // find the directory of the path
    if (vfs_lookup_parent(path, &dir, &filename) != 0)
      panic("vfs_open: failed to lookup parent!\n");

    // create a file in the directory
    if (vop_create(dir, filename, &node) != 0)
      panic("vfs_open: failed to create file!\n");
  }

  ++node->ref;
  sprint("vfs_open: inode ref: %d\n", node->ref);

  *inode_store = node;
  return 0;
}

//
// lookup the path
//
int vfs_lookup(char *path, struct inode **node_store) {
  struct inode *dir;
  int ret = get_device(path, &path, &dir);
  // Case 1: use host device file system
  if (ret == -1) {
    *node_store = NULL;
    return -1;
  }
  // Case 2: use PKE file system, dir: device root-dir-inode
  // device:/.../...

  // given root-dir-inode, find the file-inode in $path
  ret = vop_lookup(dir, path, node_store);
  return ret;
}

/*
 * get_device: 根据路径，选择目录的inode存入node_store
 * path format (absolute path):
 *    Case 1: device:path
 *      <e.g.> ramdisk0:/fileinram.txt
 *    Case 2: path
 *      <e.g.> fileinhost.txt
 * @return
 *    -1:     host device
 *    else:   save device root-dir-inode into node_store
 */
int get_device(char *path, char **subpath, struct inode **node_store) {

  int colon = -1;
  //scan the path
  for (int i = 0; path[i] != '\0'; ++i) {
    if (path[i] == ':') {
      colon = i;
      break;
    }
  }
  // Case 2: the host device is used by default
  if (colon < 0) {
    *subpath = path;
    *node_store = NULL;
    return -1;
  }

  // Case 1: find the root node of the device in the vdev_list
  // get device name
  char devname[32];
  path[colon] = '\0'; //device:/filename -> devname=decive
  strcpy(devname, path);
  path[colon] = ':';
  *subpath = path + colon + 2; // device:/filename -> subpath=filename
  // get the root dir-inode of [the device named "path"]
  return vfs_get_root(devname, node_store);
}

//
// vfs_get_root
//
int vfs_get_root(const char *devname, struct inode **root_store) {
  vfs_dev_t *devp=NULL;
  // find the device entry in vfs_device_list(vdev_list) named devname
  for (int i = 0; i < MAX_DEV; i++) {
    if (strcmp(vdev_list[i]->devname, devname) == 0) {
      devp = vdev_list[i];
      break;
    }
  }
  
  if (devp == NULL)
    panic("vfs_get_root: Cannot find the device entry!\n");

  // call fs.fs_get_root()
  struct inode *rootdir = fsop_get_root(devp->fs);
  if (rootdir == NULL)
    panic("vfs_get_root: failed to get root dir inode!\n");
  *root_store = rootdir;
  return 0;
}

/*
 * lookup the directory of the given path
 * @param path:       the path must be in the format of device:path
 * @param node_store: store the directory inode
 * @param fn:         file name
 */
int vfs_lookup_parent(char *path, struct inode **node_store, char **filename) {
  int ret;
  struct inode *dir;
  ret = get_device(path, filename, &dir); // get file name

  if (ret == -1)
    panic("vfs_lookup_parent: unexpectedly lead to host device!\n");

  *node_store = dir; // get dir inode
  return 0;
}