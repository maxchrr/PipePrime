#include <stdio.h>

#include "io_utils.h"
#include "myassert.h"

void create_fd(const char* PROCESS, int* fd, const char* name)
{
	ssize_t ret = pipe(fd);
	myassert(ret != -1, "'pipe' -> impossible de créer le tube");
	printf("[%s] Création du tube %d (%s)\n", PROCESS, *fd, name);
}

void dispose_fd(const char* PROCESS, const int fd, const char* name)
{
	ssize_t ret = close(fd);
	myassert(ret == 0, "'close' -> impossible de fermer le tube");
	printf("[%s] Fermeture du tube %d (%s)\n", PROCESS, fd, name);
}

ssize_t reader(int fd, void* buf, size_t size) {
	size_t total = 0;
	char* ptr = buf;
	ssize_t ret = -1;
	while (ret != 0)
	{
		ret = read(fd, ptr + total, size - total);
		myassert(ret != -1, "'read' -> erreur à la lecture du fd");
		total += ret;
	}
	return total;
}

ssize_t writer(int fd, const void* buf, size_t size) {
	size_t total = 0;
	const char* ptr = buf;
	ssize_t ret = -1;
	while (ret != 0)
	{
		ret = write(fd, ptr + total, size - total);
		myassert(ret != -1, "'write' -> erreur à l'écriture du fd");
		total += ret;
	}
	return total;
}
