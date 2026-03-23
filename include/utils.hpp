#pragma once
#include <fcntl.h>

namespace qiyi {
void setNonblock(int fd) {
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}
}  // namespace qiyi
