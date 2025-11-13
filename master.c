#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include "myassert.h"

#include "master_client.h"
#include "master_worker.h"

#include "io_utils.h"

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le master
// a besoin
struct master
{
    /* data */
    int highest_prime;
    int how_many_prime;
};


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
void loop(struct master data, int semId)
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
    bool stop = false;
    do
    {
        ssize_t ret;

        struct sembuf entree_attmut_client =    {1, 1, 0};
	    struct sembuf entree_attmut_master =    {2, -1, 0};

        ret = semop(semId, &entree_attmut_client, 1);
        myassert(ret != -1, "erreur : semop : entree_attmut_client");

        ret = semop(semId, &entree_attmut_master, 1);
        myassert(ret != -1, "erreur : semop : entree_attmut_master");

        int fd_client_master = open_fifo("fd_client_master", O_RDONLY);
        int fd_master_client = open_fifo("fd_master_client", O_WRONLY);

        // TODO: tubes anonymes worker

        int d;
        ret = reader(fd_client_master, &d, sizeof(int));

        printf("ORDER: %d\n", d);

        if (d == ORDER_STOP)
        {
            // TODO: communication worker

            const char* msg = "EOC\n";
            ret = writer(fd_master_client, msg, strlen(msg));

            stop = true;
        }
        else if (d == ORDER_COMPUTE_PRIME)
        {
            int n;
            ret = reader(fd_client_master, &n, sizeof(int));
        }
        else if (d == ORDER_HOW_MANY_PRIME)
        {
            int n = data.how_many_prime;
            ret = reader(fd_client_master, &n, sizeof(int));
        }
        else if (d == ORDER_HIGHEST_PRIME)
        {
            int n = data.highest_prime;
            ret = reader(fd_client_master, &n, sizeof(int));
        }

        close_fifo(fd_client_master, "fd_client_master");
        close_fifo(fd_master_client, "fd_master_client");

        // TODO: tubes anonymes worker
    } while (!stop);
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char * argv[])
{
    struct master data = { .highest_prime = -1, .how_many_prime = -1 };

    if (argc != 1)
        usage(argv[0], NULL);

    // - création des sémaphores
    // - création des tubes nommés
    // - création du premier worker
    key_t key;
    int semId;
    ssize_t ret;

    key = ftok(IPC_PATH_NAME, IPC_ID);
    myassert(key != -1, "erreur : ftok - file doesn't exist");

    semId = semget(key, IPC_SIZE, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semId != -1, "erreur : semget - unable to create the semaphore");

    unsigned short values[IPC_SIZE];
    values[0] = 1 ; // sémaphore 0 (SC des clients) à 1
    values[1] = 0; // sémaphore 1 (attente mutuelle client) à 0
    values[2] = 0; // sémaphore 2 (attente mutuelle master) à 0

    union semun {
            int                 val;    /* value for SETVAL */
            struct semid_ds     *buf;   /* buffer for IPC_STAT & IPC_SET */
            unsigned short      *array; /* array for GETALL & SETALL */
            struct seminfo      *__buf; /* buffer for IPC_INFO (Linux-specific */
    } arg;

    arg.array = values;

    ret = semctl(semId, 0, SETALL, arg);
    myassert(ret != -1, "erreur : semctl - unable to initialize the semaphore");

    create_fifo("fd_client_master"); // tube 0 (client -> master)
    create_fifo("fd_master_client"); // tube 1 (master -> client)

    // boucle infinie
    loop(data, semId);

    // destruction des tubes nommés, des sémaphores, ...
    dispose_fifo("fd_client_master");
    dispose_fifo("fd_master_client");

    ret = semctl(semId, -1, IPC_RMID);
    myassert(semId != -1, "erreur : semctl - unable to destroy the semaphore");

    return EXIT_SUCCESS;
}

// N'hésitez pas à faire des fonctions annexes ; si les fonctions main
// et loop pouvaient être "courtes", ce serait bien
