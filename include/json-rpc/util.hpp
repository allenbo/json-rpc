#ifndef __JSONPRC_UTIL_HPP__
#define __JSONPRC_UTIL_HPP__
#include <fcntl.h>

static inline void nonblock_fd(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  flag = flag | O_NONBLOCK;
  fcntl(fd, F_SETFL, flag);
}

static inline void block_df(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  flag = flag & ~O_NONBLOCK;
  fcntl(fd, F_SETFL, flag);
}

static const int UNINIT_SOCKET = -1;

#endif
