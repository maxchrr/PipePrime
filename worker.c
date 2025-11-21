#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>

#include <sys/wait.h>

#include "myassert.h"

#include "master_worker.h"

#include "io_utils.h"

#define PROCESS "WORKER"

/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le worker
// a besoin : le nombre premier dont il a la charge, ...
struct worker
{
	/* data */
	int number;
	int fdIn;
	int fdToMaster;
	int* fdToWorker;
};

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void usage(const char *exeName, const char *message)
{
	fprintf(stderr, "usage : %s <n> <fdIn> <fdToMaster>\n", exeName);
	fprintf(stderr, "   <n> : nombre premier géré par le worker\n");
	fprintf(stderr, "   <fdIn> : canal d'entrée pour tester un nombre\n");
	fprintf(stderr, "   <fdToMaster> : canal de sortie pour indiquer si un nombre est premier ou non\n");
	if (message != NULL)
		fprintf(stderr, "message : %s\n", message);
	exit(EXIT_FAILURE);
}

static void parseArgs(int argc, char * argv[], struct worker* current_worker)
{
	if (argc != 4)
		usage(argv[0], "Nombre d'arguments incorrect");

	// remplir la structure
	current_worker->number = atoi(argv[1]);
	current_worker->fdIn = atoi(argv[2]);
	current_worker->fdToMaster = atoi(argv[3]);
	current_worker->fdToWorker = NULL;
}

/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/

void loop(bool res, struct worker* current_worker)
{
	// boucle infinie :
	//    attendre l'arrivée d'un nombre à tester
	//    si ordre d'arrêt
	//       si il y a un worker suivant, transmettre l'ordre et attendre sa fin
	//       sortir de la boucle
	//    sinon c'est un nombre à tester, 4 possibilités :
	//           - le nombre est premier
	//           - le nombre n'est pas premier
	//           - s'il y a un worker suivant lui transmettre le nombre
	//           - s'il n'y a pas de worker suivant, le créer
	bool stop = false;
	do
	{
		ssize_t ret;
		int d;
		reader(current_worker->fdIn, &d, sizeof(int));

		// Ordre d'arrêt
		if (d == -1)
		{
			// Arrêter tout les workers suivants
			if (current_worker->fdToWorker != NULL) // TODO: while ou if ?
			{
				ret = writer(*(current_worker->fdToWorker), &d, sizeof(int));
				ret = wait(NULL);
				myassert(ret != -1, "erreur : wait");
			}
			stop = true;
		}
		else
		{
		if (d == current_worker->number)  // si le nombre correspond au numéro du worker (donc premier)
		{
			writer(current_worker->fdToMaster, &d, sizeof(int));
		}
		else if (d % current_worker->number == 0) // si il n'est pas premier
		{
			res = false;
			writer(current_worker->fdToMaster, &res, sizeof(bool));
		}
		else  // il faut le donner au worker suivant
		{
			if (current_worker->fdToWorker != NULL) // il en existe un (on lui donne le nombre)
			{
				writer(*(current_worker->fdToWorker), &d, sizeof(int));
			}
			else // il n'en existe pas (on en créé un)
			{
				int fds_master_worker[2];
				create_fd(PROCESS, fds_master_worker,"fds_me_worker");
				current_worker->fdToWorker = &fds_master_worker[1];  // écrivain (on chaîne le worker)

				// duplication
				ret = fork_process();

				// fils
				if (ret == 0)
				{
					dispose_fd(PROCESS, fds_master_worker[1], "fds_master_worker");

					launch_worker(d, fds_master_worker[0], current_worker->fdToMaster);
				}

				dispose_fd(PROCESS, fds_master_worker[0], "fds_master_worker");

				writer(*(current_worker->fdToWorker), &d, sizeof(int));
			}
		}
		}
	} while (!stop);
}

/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
	bool res = true;
	struct worker* current_worker = malloc(sizeof(struct worker));
	parseArgs(argc, argv , current_worker);

	// Si on est créé c'est qu'on est un nombre premier
	// Envoyer au master un message positif pour dire
	// que le nombre testé est bien premier
	writer(current_worker->fdToMaster, &res, sizeof(bool));

	loop(res, current_worker);

	// libérer les ressources : fermeture des files descriptors par exemple
	dispose_fd(PROCESS, current_worker->fdIn, "worker");
	dispose_fd(PROCESS, current_worker->fdToMaster, "worker");
	free(current_worker);

	return EXIT_SUCCESS;
}
