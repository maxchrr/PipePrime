#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <unistd.h>

void create_fd(int* fd, const char* name);
void dispose_fd(const int fd, const char* name);

ssize_t reader(int fd, void* buf, size_t size);
ssize_t writer(int fd, const void* buf, size_t size);

#endif
