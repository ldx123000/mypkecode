#include "vfs.h"
#include "pmm.h"
#include "spike_interface/spike_utils.h"
#include "util/string.h"

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
    panic("fail to find the device entry!\n");

  // mount the specific file system to the device with mountfunc
  if (mountfunc(devp->dev, &(devp->fs)) == 0)
    sprint("file system successfully mount to %s\n", devp->devname);
  return 0;
}

//
// get an inode from given path
//
int vfs_open(char *path, int open_flags, struct inode **inode_store) {
  // lookup the path, and create an related inode
  inode *node;
  int ret = vfs_lookup(path, &node);

  // if the path belongs to the PKE files device
  if (ret == 1) { // file doesn't exists(ret==1)
    int creatable = open_flags & O_CREATE;
    if (creatable == 0)
      panic("uncreatable file!\n");

    // create a file, and get its inode
    char *filename; // file name
    inode *dir;     // dir inode
    // find the directory of the path
    vfs_lookup_parent(path, &dir, &filename);

    // create a file in the directory
    if (vop_create(dir, filename, &node) != 0)
      panic("failed to create file!\n");
  }

  ++node->ref_count;
  //sprint("inode ref_count: %d\n", node->ref_count);

  *inode_store = node;
  return 0;
}

//
// lookup the directory of the path
//
int vfs_lookup_parent(char *path, struct inode **node_store, char **filename) {
  struct inode *dir;
  int ret = get_device(path, filename, &dir); // get file name
  if (ret == -1)
    panic("format error!\n");
  *node_store = dir; // get dir inode
  return 0;
}

//
// lookup the path
//
int vfs_lookup(char *path, struct inode **node_store) {
  struct inode *dir;
  int ret = get_device(path, &path, &dir);
  // wrong format
  if (ret == -1) {
    panic("format error!\n");
    return -1;
  }
  // given root directory inode to find the inode
  ret = vop_lookup(dir, path, node_store);
  return ret;
}

//
//   save device root directory inode into node_store
//
int get_device(char *path, char **subpath, struct inode **node_store) {
  int idx = -1;
  // scan the path
  for (int i = 0; path[i] != '\0'; ++i) {
    if (path[i] == ':') {
      idx = i;
      break;
    }
  }
  // wrong format
  if (idx < 0) {
    *subpath = path;
    *node_store = NULL;
    panic("wrong format!");
    return -1;
  }

  // find the root node of the device in the vdev_list
  // get device name
  char devname[MAX_DEVNAME];
  path[idx] = '\0'; // device:/filename -> devname=decive
  strcpy(devname, path);

  path[idx] = ':';
  *subpath = path + idx + 2; // device:/filename -> subpath=filename
  // get the root directory inode
  return vfs_get_root(devname, node_store);
}

//
// try to get root inode by devname
//
int vfs_get_root(const char *devname, inode **root_store) {
  vfs_dev_t *devp = NULL;
  // find the device entry in vfs_device_list
  for (int i = 0; i < MAX_DEV; i++) {
    if (strcmp(vdev_list[i]->devname, devname) == 0) {
      devp = vdev_list[i];
      break;
    }
  }
  if (devp == NULL)
    panic("no device entry meets demands!\n");

  // call fs.fs_get_root()
  inode *rootdir = fs_op_get_root(devp->fs);
  if (rootdir == NULL)
    panic("failed to get root dir inode!\n");
  *root_store = rootdir;
  return 0;
}