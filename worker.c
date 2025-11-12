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

/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le worker
// a besoin : le nombre premier dont il a la charge, ...
struct worker
{
    /* data */
    int number;
    int input_channel;
    int master_channel;
    int next_worker_channel;
} worker;


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
    (*current_worker).number = atoi(argv[1]);
    (*current_worker).input_channel = atoi(argv[2]);
    (*current_worker).master_channel = atoi(argv[3]);
    (*current_worker).next_worker_channel = NULL;
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
    do {
    	int c;
    	int ret;

    	ret = reader((*current_worker).input_channel, &c, sizeof(int));
    
    	if (c == -1)
        {
    		if (current_worker->next_worker_channel != NULL)
            { 
    			ret = writer((*current_worker).next_worker_channel, &c, sizeof(int));
    			
    			ret = wait(NULL);
        		myassert(ret != -1, "erreur : wait");
    		}
    		stop = true;
    	}
        else
        {
    		bool res = false;
    		if (c == current_worker->number)
            {
    			res = true;
    			ret = writer((*current_worker).master_channel, &res, sizeof(bool));
    		}
            else if (c % current_worker->number == 0)
            {
    			ret = writer((*current_worker).master_channel, &res, sizeof(bool));
    		}
            else {
    		}		
        }
    } while (!stop);
    
}

/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    struct worker current_worker;
    parseArgs(argc, argv , &current_worker);
    
    // Si on est créé c'est qu'on est un nombre premier
    // Envoyer au master un message positif pour dire
    // que le nombre testé est bien premier

    loop(&current_worker);

    // libérer les ressources : fermeture des files descriptors par exemple

    return EXIT_SUCCESS;
}
