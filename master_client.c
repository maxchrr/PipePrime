#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "myassert.h"

#include "master_client.h"

// fonctions éventuelles internes au fichier
void create_fifo(const char* PROCESS, const char* name)
{
	ssize_t ret = mkfifo(name, 0644);
	myassert(ret == 0, "'mkfifo' -> impossible de créer le fifo");
	printf("[%s] Création du fifo %s\n", PROCESS, name);
}

void dispose_fifo(const char* PROCESS, const char* name)
{
	ssize_t ret = unlink(name);
	myassert(ret == 0, "'unlink' -> impossible de détruire le fifo");
	printf("[%s] Destruction du fifo %s\n", PROCESS, name);
}

int open_fifo(const char* PROCESS, const char* name, int mode)
{
	int fd = open(name, mode);
	myassert(fd != -1, "'open' - impossible d'ouvrir le fifo");
	printf("[%s] Le fifo %s est ouvert\n", PROCESS, name);
	return fd;
}

void close_fifo(const char* PROCESS, int fd, const char* name)
{
	ssize_t ret = close(fd);
	myassert(ret == 0, "'close' -> impossible de fermer le fifo");
	printf("[%s] Le fifo %s est fermé\n", PROCESS, name);
}

// fonctions éventuelles proposées dans le .h

