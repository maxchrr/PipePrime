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
#include <sys/wait.h>

#include "myassert.h"

#include "master_client.h"
#include "master_worker.h"

#include "io_utils.h"

#define PROCESS "MASTER"

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le master
// a besoin
struct master
{
	/* data */
	int fdIn;
	int fdOut;
	int semId;
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

void loop(struct master data)
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

		struct sembuf wait_point_client =  {1, 1, 0};
		struct sembuf wait_point_master =  {2, -1, 0};

		op_ipc(data.semId, &wait_point_client, 1);
		op_ipc(data.semId, &wait_point_master, 1);

		int fd_client_master = open_fifo(PROCESS, "fd_client_master", O_RDONLY);
		int fd_master_client = open_fifo(PROCESS, "fd_master_client", O_WRONLY);

		int order;
		reader(fd_client_master, &order, sizeof(int));

		if (order == ORDER_STOP)
		{
			// ordre d'arrêt du worker
			const int os = -1;
			writer(data.fdOut, &os, sizeof(int));

			ret = wait(NULL);
			myassert(ret != -1, "'wait'");

			// accusé de réception pour le client
			const char* msg = "EOC\n";
			writer(fd_master_client, msg, strlen(msg));

			stop = true;
		}
		else if (order == ORDER_COMPUTE_PRIME)
		{
			/* Lecture du nombre demandé */
			int n;
			reader(fd_client_master, &n, sizeof(int));

			/* Calcul du résultat */
			bool res;
			if (n > data.highest_prime)
			{
				// On calcule N
				for (int i=data.highest_prime+1; i<=n; ++i)
				{
					writer(data.fdOut, &i, sizeof(int));
					reader(data.fdIn, &res, sizeof(bool));

					if (res)
					{
						data.highest_prime = i;
						data.how_many_prime += 1;
					}
				}
			}
			else
			{
				// N est connu
				writer(data.fdOut, &n, sizeof(int));
				reader(data.fdIn, &res, sizeof(bool));
			}
			
			/* Réponse au client */
			writer(fd_master_client, &res, sizeof(bool));
		}
		else if (order == ORDER_HOW_MANY_PRIME)
		{
			writer(fd_master_client, &data.how_many_prime, sizeof(int));
		}
		else if (order == ORDER_HIGHEST_PRIME)
		{
			writer(fd_master_client, &data.highest_prime, sizeof(int));
		}

		close_fifo(PROCESS, fd_client_master, "fd_client_master");
		close_fifo(PROCESS, fd_master_client, "fd_master_client");
	} while (!stop);
}

/************************************************************************
 * Fonctions annexes
 ************************************************************************/

int create_ipc(const char* path_name, const int id, const int size, const int flags)
{
	key_t key;
	int semId;

	key = ftok(path_name, id);
	myassert(key != -1, "'ftok' -> le descripteur n'existe pas");

	semId = semget(key, size, flags);
	myassert(semId != -1, "'semget' -> impossible de créer le sémaphore");

	return semId;
}

void init_ipc(const int semId, unsigned short values[])
{
	ssize_t ret;
	union semun {
		int			val;	/* value for SETVAL */
		struct semid_ds		*buf;	/* buffer for IPC_STAT & IPC_SET */
		unsigned short		*array;	/* array for GETALL & SETALL */
		struct seminfo		*__buf;	/* buffer for IPC_INFO (Linux-specific */
	} arg;

	arg.array = values;

	ret = semctl(semId, 0, SETALL, arg);
	myassert(ret != -1, "'semctl' -> impossible d'initialiser le sémaphore");
}

void dispose_ipc(const int semId)
{
	ssize_t ret;
	ret = semctl(semId, -1, IPC_RMID);
	myassert(ret != -1, "'semctl' -> impossible de détruire le sémaphore");
}

/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char * argv[])
{
	ssize_t ret;
	struct master data = {
		.fdIn = -1,
		.fdOut = -1,
		.semId = -1,
		.highest_prime = 2,
		.how_many_prime = 1
	};

	if (argc != 1)
		usage(argv[0], NULL);

	// - création des sémaphores
	// - création des tubes nommés
	// - création du premier worker

	// sémaphores
	data.semId = create_ipc(IPC_PATH_NAME, IPC_ID, IPC_SIZE, IPC_CREAT | IPC_EXCL | 0641);

	unsigned short values[IPC_SIZE];
	values[0] = 1;	// sémaphore 0 (SC des clients) à 1
	values[1] = 0;	// sémaphore 1 (attente mutuelle client) à 0
	values[2] = 0;	// sémaphore 2 (attente mutuelle master) à 0

	init_ipc(data.semId, values);

	// fifo
	create_fifo(PROCESS, "fd_client_master");  // tube 0 (client -> master)
	create_fifo(PROCESS, "fd_master_client");  // tube 1 (master -> client)

	// déclaration du tube
	// il y aura un file descriptor par extrémité du tube :
	//    0 : extrémité en lecture   (0 comme stdin)
	//    1 : extrémité en écriture  (1 comme stdout)
	int fds_master_worker[2];
	int fds_worker_master[2];
	create_fd(PROCESS, fds_master_worker, "fds_master_worker");
	create_fd(PROCESS, fds_worker_master, "fds_worker_master");
	data.fdOut =	fds_master_worker[1];	// écrivain
	data.fdIn =	fds_worker_master[0];	// lecteur

	// duplication
	ret = fork_process();

	// fils
	if (ret == 0)
	{
		// fermer les canaux inutiles du worker (fils)
		dispose_fd(PROCESS, fds_master_worker[1], "worker");
		dispose_fd(PROCESS, fds_worker_master[0], "worker");

		launch_worker(2, fds_master_worker[0], fds_worker_master[1]);
	}

	// fermer les canaux inutiles du master (père)
	dispose_fd(PROCESS, fds_master_worker[0], "master");
	dispose_fd(PROCESS, fds_worker_master[1], "master");
	
	bool res;
	reader(data.fdIn, &res, sizeof(bool));

	printf("[%s] Prêt\n", PROCESS);
	
	// boucle infinie
	loop(data);

	// destruction des tubes nommés, des sémaphores, ...
	dispose_fd(PROCESS, fds_master_worker[1], "master");
	dispose_fd(PROCESS, fds_worker_master[0], "master");
	dispose_fifo(PROCESS, "fd_client_master");
	dispose_fifo(PROCESS, "fd_master_client");
	dispose_ipc(data.semId);
	
	printf("[%s] Arrêt\n", PROCESS);

	return EXIT_SUCCESS;
}

// N'hésitez pas à faire des fonctions annexes ; si les fonctions main
// et loop pouvaient être "courtes", ce serait bien
