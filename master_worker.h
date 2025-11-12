#ifndef MASTER_WORKER_H
#define MASTER_WORKER_H

// On peut mettre ici des éléments propres au couple master/worker :
//    - des constantes pour rendre plus lisible les comunications
//    - des fonctions communes (écriture dans un tube, ...)
ssize_t reader(int fd, void* buf, size_t size);
ssize_t writer(int fd, const void* buf, size_t size);

#endif
