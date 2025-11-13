#include "io_utils.h"
#include "myassert.h"

ssize_t reader(int fd, void* buf, size_t size) {
    size_t total = 0;
    char* ptr = buf;
    ssize_t ret = -1;
    while (ret != 0) {
        ret = read(fd, ptr + total, size - total);
        myassert(ret != -1, "Erreur : reading on fifo failed");
        total += ret;
    }
    return total;
}

ssize_t writer(int fd, const void* buf, size_t size) {
    size_t total = 0;
    const char* ptr = buf;
    ssize_t ret = -1;
    while (ret != 0) {
        ret = write(fd, ptr + total, size - total);
        myassert(ret != -1, "Erreur : writing on fifo failed");
        total += ret;
    }
    return total;
}
