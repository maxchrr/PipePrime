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
void create_fifo(const char* name)
{
    ssize_t ret = mkfifo(name, 0644);
    myassert(ret == 0, "erreur : mkfifo - cannot create the fifo");
    printf("[MASTER] Created fifo %s\n", name);
}

void dispose_fifo(const char* name) 
{
    ssize_t ret = unlink(name);
    myassert(ret == 0, "erreur : unlink - cannot remove fifo");
    printf("[MASTER] Dispose fifo %s\n", name);
}

int open_fifo(const char* name, int mode)
{
        int fd = open(name, mode);
        myassert(fd != -1, "erreur : open - cannot open the fifo");
        printf("[MASTER-CLIENT] The fifo %s is open\n", name);
        return fd;
}

void close_fifo(int fd, const char* name)
{
        ssize_t ret = close(fd);
        myassert(ret == 0, "erreur : close - cannot close fifo");
        printf("[MASTER-CLIENT] The fifo %s is close\n", name);
}

void create_fd(int* fd)
{
    ssize_t ret = pipe(fd);
    myassert(ret != -1, "erreur : pipe - cannot create the fd");
    printf("[MASTER] Created fd %d\n", *fd);
}

void dispose_fd(const int fd) 
{
    ssize_t ret = close(fd);
    myassert(ret == 0, "erreur : close - cannot close the fd");
    printf("[MASTER] Dispose fd %d\n", fd);
}

// fonctions éventuelles proposées dans le .h

