#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "myassert.h"
#include <sys/wait.h>

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
    int canalIn;
    int canalMaster;
    int canalNextWorker;
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

static void parseArgs(int argc, char * argv[] , worker * work/*, structure à remplir*/)
{
    if (argc != 4)
        usage(argv[0], "Nombre d'arguments incorrect");

    // remplir la structure
    *work = {number= argv[1]; canalIn = argv[2] ; canalMaster = argv[3]; canalNextWorker = Null };
}

/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/

void loop(worker * work /* paramètres */)
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
    	ret = read((*work).canalIn, &c, sizeof(int));
    	myassert(ret != -1 || ret == sizeof(char) || ret == 0, "erreur : read - canalIn");
    
    	if (c==-1){
    		if (work.canalNextWorker != Null){ 
    			ret = write((*work).canalNextWorker, &c, sizeof(int));
    			myassert(ret != -1 || ret == sizeof(char) || ret == 0, "erreur : write - canalNextWorker");
    			
    			ret = wait(NULL);
        		assert(ret != -1);
    		}
    		stop = true;
    	} else {
    		bool res = false;
    		if (c == work.number){
    			res = true;
    			ret = write((*work).canalMaster, &res, sizeof(bool));
    			myassert(ret != -1 || ret == sizeof(bool) || ret == 0, "erreur : write - canalMaster");
    		} else if ( c mod work.number == 0) {
    			ret = write((*work).canalMaster, &res, sizeof(bool));
    			myassert(ret != -1 || ret == sizeof(bool) || ret == 0, "erreur : write - canalMaster");
    		} else {
    			}
    			
    			
    } while (!stop);
    
}

/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    worker work ;
    parseArgs(argc, argv , &work /*, structure à remplir*/);
    
    // Si on est créé c'est qu'on est un nombre premier
    // Envoyer au master un message positif pour dire
    // que le nombre testé est bien premier

    loop(&work/* paramètres */);

    // libérer les ressources : fermeture des files descriptors par exemple

    return EXIT_SUCCESS;
}
