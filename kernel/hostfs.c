#include "hostfs.h"
#include "spike_interface/spike_utils.h"
// ///////////////////////////////////
// Access to the host file system
// ///////////////////////////////////

int host_open(char *path, int flags) {
  spike_file_t *f = spike_file_open(path, flags, 0);
  if ((int64)f < 0)
    return -1;
  int fd = spike_file_dup(f);
  if (fd < 0) {
    spike_file_decref(f);
    return -1;
  }
  return fd;
}

int host_read(int fd, char *buf, uint64 size) {
  spike_file_t *f = spike_file_get(fd);
  return spike_file_read(f, buf, size);
}

int host_write(int fd, char *buf, uint64 size) {
  spike_file_t *f = spike_file_get(fd);
  return spike_file_pwrite(f, buf, size);
}

int host_close(int fd) {
  spike_file_t *f = spike_file_get(fd);
  return spike_file_close(f);
}