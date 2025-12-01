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
#include <sys/sem.h>

#include "myassert.h"

#include "master_client.h"

// fonctions éventuelles internes au fichier

// fonctions éventuelles proposées dans le .h
void create_fifo(const char* PROCESS, const char* name)
{
	UNUSED(PROCESS);
	UNUSED(name);

	ssize_t ret = mkfifo(name, 0644);
	myassert(ret == 0, "'mkfifo' -> impossible de créer le fifo");
	TRACE("[%s] Création du fifo %s\n", PROCESS, name);
}

void dispose_fifo(const char* PROCESS, const char* name)
{
	UNUSED(PROCESS);
	UNUSED(name);

	ssize_t ret = unlink(name);
	myassert(ret == 0, "'unlink' -> impossible de détruire le fifo");
	TRACE("[%s] Destruction du fifo %s\n", PROCESS, name);
}

int open_fifo(const char* PROCESS, const char* name, int mode)
{
	UNUSED(PROCESS);
	UNUSED(name);

	int fd = open(name, mode);
	myassert(fd != -1, "'open' - impossible d'ouvrir le fifo");
	TRACE("[%s] Le fifo %s est ouvert\n", PROCESS, name);
	return fd;
}

void close_fifo(const char* PROCESS, int fd, const char* name)
{
	UNUSED(PROCESS);
	UNUSED(name);

	ssize_t ret = close(fd);
	myassert(ret == 0, "'close' -> impossible de fermer le fifo");
	TRACE("[%s] Le fifo %s est fermé\n", PROCESS, name);
}

void op_ipc(const int semId, struct sembuf* ops, size_t n)
{
	ssize_t ret;
	ret = semop(semId, ops, n);
	myassert(ret != -1, "'semop''");
}
