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

void loop(struct worker* current_worker)
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
		int c;
		ssize_t ret;
		
		ret = reader(current_worker->fdIn, &c, sizeof(int));
		printf("%d\n",c);
		if (c == -1) //ordre d'arêt
		{
		
		if (current_worker->fdToWorker != NULL)
			{ 
			ret = writer(*(current_worker->fdToWorker), &c, sizeof(int));
			
			ret = wait(NULL);
			myassert(ret != -1, "erreur : wait");
			
			}
		
		stop = true;
		}
		else
		{
			bool res = false;
			if (c == current_worker->number)  // si c'est le même nombre (donc premier)
			{
				res = true;
				ret = writer(current_worker->fdToMaster, &res, sizeof(bool));
			}
			else if (c % current_worker->number == 0) // si il n'est pas premier
			{
				ret = writer(current_worker->fdToMaster, &res, sizeof(bool));
			}
			else  // il faut le donner au worker suivant
			{
				if (current_worker->fdToWorker != NULL) //il en existe 1
				{ 
					ret = writer(*(current_worker->fdToWorker), &c, sizeof(int));
				}
				else // il n'en existe pas 
				{
					
					int fds_me_worker[2];
					
					create_fd(fds_me_worker,"fds_me_worker");
					
					current_worker->fdToWorker = &fds_me_worker[1];  // écrivain

					ret = fork();
					myassert(ret != -1, "erreur : fork");
				
					if (ret == 0)
					{
						ret = close(fds_me_worker[1]);
						myassert(ret == 0, "erreur : close - cannot close the pipe");
					
						char buffer[1000];
						char* argv[5];
						argv[0] = "./worker";
						sprintf(buffer, "%d", c);   // int to char*
						argv[1] = buffer;
						sprintf(buffer, "%d", fds_me_worker[0]);
						argv[2] = buffer;
						sprintf(buffer, "%d", current_worker->fdToMaster);
						argv[3] = buffer;
						argv[4] = NULL;
					
						ret = execv("./worker",argv);
					}
				
					ret = close(fds_me_worker[0]);
					myassert(ret == 0, "erreur : close - cannot close the pipe");
				
					ret = writer(*(current_worker->fdToWorker), &c, sizeof(int));
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
    
	struct worker* current_worker = malloc(sizeof(struct worker));
	parseArgs(argc, argv , current_worker);
	
	// Si on est créé c'est qu'on est un nombre premier
	// Envoyer au master un message positif pour dire
	// que le nombre testé est bien premier
	ssize_t ret;
	bool res = true;
	ret = writer(current_worker->fdToMaster, &res, sizeof(bool));
	myassert(ret != -1, "erreur : writer");
	loop(current_worker);
		
	// libérer les ressources : fermeture des files descriptors par exemple
	dispose_fd(current_worker->fdIn,"fds_master_worker - fils");
	dispose_fd(current_worker->fdToMaster,"fds_worker_master - fils");
	free(current_worker);

	return EXIT_SUCCESS;
}
