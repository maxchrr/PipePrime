#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include "myassert.h"

#include "master_worker.h"

// fonctions éventuelles internes au fichier

// fonctions éventuelles proposées dans le .h
ssize_t fork_process(void)
{
	ssize_t ret;
	ret = fork();
	myassert(ret != -1, "'fork' -> duplication impossible");
	return ret;
}

void launch_worker(const int n, const int fdIn, const int fdOut)
{
	ssize_t ret;

	char buffer[1000], fd_in[16], fd_out[16];
	snprintf(buffer, sizeof(buffer), "%d", n);
	snprintf(fd_in, sizeof(fd_in), "%d", fdIn);
	snprintf(fd_out, sizeof(fd_out), "%d", fdOut);

	char *argv[] = {
		"./worker",
		buffer,
		fd_in,
		fd_out,
		NULL
	};

	ret = execv("./worker", argv);
	myassert(ret == 0, "'execv' -> impossible de lancer le worker");
}
