#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>

#include "myassert.h"

#include "master_client.h"

// fonctions éventuelles internes au fichier
int open_fifo(const char* name, int mode)
{
        int fifo = open(name, mode);
        myassert(fifo != -1, "erreur : open - cannot open the fifo");
        printf("[MASTER-CLIENT] The fifo %s is open\n", name);
        return fifo;
}

void close_fifo(int fifo, const char* name)
{
        int ret = close(fifo);
        myassert(ret == 0, "erreur : close - cannot close fifo");
        printf("[MASTER-CLIENT] The fifo %s is close\n", name);
}

// fonctions éventuelles proposées dans le .h

