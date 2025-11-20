#include <stdio.h>

#include "io_utils.h"
#include "myassert.h"

void create_fd(int* fd, const char* name)
{
	ssize_t ret = pipe(fd);
	myassert(ret != -1, "erreur : pipe - cannot create the fd");
	printf("[MASTER] Created fd %d (%s)\n", *fd, name);
}

void dispose_fd(const int fd, const char* name) 
{
	ssize_t ret = close(fd);
	myassert(ret == 0, "erreur : close - cannot close the fd");
	printf("[MASTER] Dispose fd %d (%s)\n", fd, name);
}

ssize_t reader(int fd, void* buf, size_t size) {
	size_t total = 0;
	char* ptr = buf;
	ssize_t ret = -1;
	while (ret != 0) {
		ret = read(fd, ptr + total, size - total);
		myassert(ret != -1, "Erreur : reading on fd failed");
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
		myassert(ret != -1, "Erreur : writing on fd failed");
		total += ret;
	}
	return total;
}
