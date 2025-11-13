#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <unistd.h>

ssize_t reader(int fd, void* buf, size_t size);
ssize_t writer(int fd, const void* buf, size_t size);

#endif
