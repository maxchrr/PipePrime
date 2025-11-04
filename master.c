#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

#include "myassert.h"

#include "master_client.h"
#include "master_worker.h"

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le master
// a besoin


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(/* paramètres */)
{
    // boucle infinie :
    // - ouverture des tubes (cf. rq client.c)
    // - attente d'un ordre du client (via le tube nommé)
    // - si ORDER_STOP
    //       . envoyer ordre de fin au premier worker et attendre sa fin
    //       . envoyer un accusé de réception au client
    // - si ORDER_COMPUTE_PRIME
    //       . récupérer le nombre N à tester provenant du client
    //       . construire le pipeline jusqu'au nombre N-1 (si non encore fait) :
    //             il faut connaître le plus grand nombre (M) déjà enovoyé aux workers
    //             on leur envoie tous les nombres entre M+1 et N-1
    //             note : chaque envoie déclenche une réponse des workers
    //       . envoyer N dans le pipeline
    //       . récupérer la réponse
    //       . la transmettre au client
    // - si ORDER_HOW_MANY_PRIME
    //       . transmettre la réponse au client (le plus simple est que cette
    //         information soit stockée en local dans le master)
    // - si ORDER_HIGHEST_PRIME
    //       . transmettre la réponse au client (le plus simple est que cette
    //         information soit stockée en local dans le master)
    // - fermer les tubes nommés
    // - attendre ordre du client avant de continuer (sémaphore : précédence)
    // - revenir en début de boucle
    //
    // il est important d'ouvrir et fermer les tubes nommés à chaque itération
    // voyez-vous pourquoi ?
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char * argv[])
{
    if (argc != 1)
        usage(argv[0], NULL);

    // - création des sémaphores
    // - création des tubes nommés
    // - création du premier worker
    key_t key = ftok("./master_client.h", 1);
    myassert(key != -1, "Erreur à la création de la clé");

    int sem_size = 3;
    int sem = semget(key, sem_size, IPC_CREAT | IPC_EXCL | 0664);
    myassert(sem != -1, "Erreur à la création des sémaphores");

    unsigned short values[sem_size];
    values[0] = 1; // sémaphore 0 (SC des clients) à 1
    values[1] = 0; // sémaphore 1 (attente mutuelle client) à 0
    values[2] = 0; // sémaphore 2 (attente mutuelle master) à 0

    union semun {
             int     val;            /* value for SETVAL */
             struct  semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
             u_short *array;         /* array for GETALL & SETALL */
    } arg;

    arg.array = values;

    int res = semctl(sem, 0, SETALL, arg);
    myassert(res != -1, "Erreur à l'initialisation des sémaphores");

    int s = 4;
    int fifos[s];
    for (int i=0; i<=s; ++i)
    {
        char path[64]; // buffer
        sprintf(path, "pipeprime_%d", i);
        int ret = mkfifo(path, 0664);
        sprintf(path, "Erreur à la création du tube %d", i);
        myassert(ret == 0, path);
        fifos[i] = ret;
    }

    // boucle infinie
    loop(/* paramètres */);

    // destruction des tubes nommés, des sémaphores, ...
    res = semctl(sem, -1, IPC_RMID);
    myassert(sem != -1, "Erreur à la destruction des sémaphores");

    for (int i=0; i<=s; ++i)
    {
        char path[64]; // buffer
        sprintf(path, "pipeprime_%d", i);
        int ret = unlink(path);
        sprintf(path, "Erreur à la destruction du tube %d", i);
        myassert(ret == 0, path);
    }

    return EXIT_SUCCESS;
}

// N'hésitez pas à faire des fonctions annexes ; si les fonctions main
// et loop pouvaient être "courtes", ce serait bien
