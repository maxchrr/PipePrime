#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "myassert.h"

#include "master_worker.h"

// fonctions éventuelles internes au fichier
ssize_t reader(int fd, void* buf, size_t size) {
        size_t total = 0;
        char* ptr = buf;
        ssize_t ret = -1;
        while (ret != 0)
        {
                ret = read(fd, ptr+total, size-total);
                myassert(ret != -1, "Erreur : reading on fifo failed");
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
                ret = write(fd, ptr+total, size-total);
                myassert(ret != -1, "Erreur : writing on fifo failed");
                total += ret;
        }
        return total;
}

// fonctions éventuelles proposées dans le .h
