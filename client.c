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

// chaines possibles pour le premier paramètre de la ligne de commande
#define TK_STOP      "stop"
#define TK_COMPUTE   "compute"
#define TK_HOW_MANY  "howmany"
#define TK_HIGHEST   "highest"
#define TK_LOCAL     "local"

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <ordre> [<nombre>]\n", exeName);
    fprintf(stderr, "   ordre \"" TK_STOP  "\" : arrêt master\n");
    fprintf(stderr, "   ordre \"" TK_COMPUTE  "\" : calcul de nombre premier\n");
    fprintf(stderr, "                       <nombre> doit être fourni\n");
    fprintf(stderr, "   ordre \"" TK_HOW_MANY "\" : combien de nombres premiers calculés\n");
    fprintf(stderr, "   ordre \"" TK_HIGHEST "\" : quel est le plus grand nombre premier calculé\n");
    fprintf(stderr, "   ordre \"" TK_LOCAL  "\" : calcul de nombres premiers en local\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

static int parseArgs(int argc, char * argv[], int *number)
{
    int order = ORDER_NONE;

    if ((argc != 2) && (argc != 3))
        usage(argv[0], "Nombre d'arguments incorrect");

    if (strcmp(argv[1], TK_STOP) == 0)
        order = ORDER_STOP;
    else if (strcmp(argv[1], TK_COMPUTE) == 0)
        order = ORDER_COMPUTE_PRIME;
    else if (strcmp(argv[1], TK_HOW_MANY) == 0)
        order = ORDER_HOW_MANY_PRIME;
    else if (strcmp(argv[1], TK_HIGHEST) == 0)
        order = ORDER_HIGHEST_PRIME;
    else if (strcmp(argv[1], TK_LOCAL) == 0)
        order = ORDER_COMPUTE_PRIME_LOCAL;
    
    if (order == ORDER_NONE)
        usage(argv[0], "ordre incorrect");
    if ((order == ORDER_STOP) && (argc != 2))
        usage(argv[0], TK_STOP" : il ne faut pas de second argument");
    if ((order == ORDER_COMPUTE_PRIME) && (argc != 3))
        usage(argv[0], TK_COMPUTE " : il faut le second argument");
    if ((order == ORDER_HOW_MANY_PRIME) && (argc != 2))
        usage(argv[0], TK_HOW_MANY" : il ne faut pas de second argument");
    if ((order == ORDER_HIGHEST_PRIME) && (argc != 2))
        usage(argv[0], TK_HIGHEST " : il ne faut pas de second argument");
    if ((order == ORDER_COMPUTE_PRIME_LOCAL) && (argc != 3))
        usage(argv[0], TK_LOCAL " : il faut le second argument");
    if ((order == ORDER_COMPUTE_PRIME) || (order == ORDER_COMPUTE_PRIME_LOCAL))
    {
        *number = strtol(argv[2], NULL, 10);
        if (*number < 2)
             usage(argv[0], "le nombre doit être >= 2");
    }       
    
    return order;
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char * argv[])
{
    int number = 0;
    int order = parseArgs(argc, argv, &number);
    printf("%d\n", order); // pour éviter le warning

    // order peut valoir 5 valeurs (cf. master_client.h) :
    //      - ORDER_COMPUTE_PRIME_LOCAL
    //      - ORDER_STOP
    //      - ORDER_COMPUTE_PRIME
    //      - ORDER_HOW_MANY_PRIME
    //      - ORDER_HIGHEST_PRIME
    //
    // si c'est ORDER_COMPUTE_PRIME_LOCAL
    //    alors c'est un code complètement à part multi-thread
    // sinon
    //    - entrer en section critique :
    //           . pour empêcher que 2 clients communiquent simultanément
    //           . le mutex est déjà créé par le master
    //    - ouvrir les tubes nommés (ils sont déjà créés par le master)
    //           . les ouvertures sont bloquantes, il faut s'assurer que
    //             le master ouvre les tubes dans le même ordre
    //    - envoyer l'ordre et les données éventuelles au master
    //    - attendre la réponse sur le second tube
    //    - sortir de la section critique
    //    - libérer les ressources (fermeture des tubes, ...)
    //    - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
    // 
    // Une fois que le master a envoyé la réponse au client, il se bloque
    // sur un sémaphore ; le dernier point permet donc au master de continuer
    //
    // N'hésitez pas à faire des fonctions annexes ; si la fonction main
    // ne dépassait pas une trentaine de lignes, ce serait bien.
    
    if (order == ORDER_COMPUTE_PRIME_LOCAL)
    {

    }
    else 
    {
        key_t key;
        int semId;
    	int ret;
    	
    	key = ftok(IPC_PATH_NAME, IPC_ID);
        myassert(key != -1, "erreur : ftok - le fichier n'existe pas ou n'est pas accessible");
    
    	semId = semget(key, IPC_SIZE, 0);
	    myassert(semId != -1, "erreur : semget");
	
        // entrée SC
        struct sembuf entree_critique_client =  {0, -1, 0};
        struct sembuf entree_attmut_client =    {1, -1, 0};
	struct sembuf entree_attmut_master =    {2, 1, 0};
    	
    	ret = semop(semId, &entree_critique_client, 1);
    	myassert(ret != -1, "erreur : semop : entree_critique_client");
    	
    	ret = semop(semId, &entree_attmut_master, 1);
        myassert(ret != -1, "erreur : semop : entree_attmut_master");
	
	ret = semop(semId, &entree_attmut_client, 1);
        myassert(ret != -1, "erreur : semop : entree_attmut_client");

        // ouverture tubes
        int fd_client_master = open_fifo("fd_client_master", O_WRONLY);
        int fd_master_client = open_fifo("fd_master_client", O_RDONLY);
        
        if (order == ORDER_STOP)
        {  
            ret = write(fd_client_master, &order, sizeof(int));
            myassert(ret != -1 || ret == sizeof(int) || ret == 0, "erreur : write - fd_client_master");

            do
            {
                char c;
                ret = read(fd_master_client, &c, sizeof(char));
                myassert(ret != -1 || ret == sizeof(char) || ret == 0, "erreur : read - fd_master_client");
                putchar(c);
            } while (ret != 0);
        }
        else if (order == ORDER_COMPUTE_PRIME)
        {
            ret = write(fd_client_master, &order, sizeof(int));
            myassert(ret != -1 || ret == sizeof(int) || ret == 0, "erreur : write - fd_client_master");
            
            char c;
            ret = read(fd_master_client, &c, sizeof(char));
            myassert(ret != -1 || ret == sizeof(char) || ret == 0, "erreur : read - fd_master_client");
        }
        else if (order == ORDER_HOW_MANY_PRIME)
        {
            ret = write(fd_client_master, &order, sizeof(int));
            myassert(ret != -1 || ret == sizeof(int) || ret == 0, "erreur : write - fd_client_master");
            
            char c;
            ret = read(fd_master_client, &c, sizeof(char));
            myassert(ret != -1 || ret == sizeof(char) || ret == 0, "erreur : read - fd_master_client");
        }
        else if (order == ORDER_HIGHEST_PRIME)
        {
            ret = write(fd_client_master, &order, sizeof(int));
            myassert(ret != -1 || ret == sizeof(int) || ret == 0, "erreur : write - fd_client_master");
            
            char c;
            ret = read(fd_master_client, &c, sizeof(char));
            myassert(ret != -1 || ret == sizeof(char) || ret == 0, "erreur : read - fd_master_client");
        }

        // sortie SC
        struct sembuf sortie_critique_client =  {0, 1, 0};
    	
    	ret = semop(semId, &sortie_critique_client, 1);
    	myassert(ret != -1, "erreur : semop : sortie_critique_client");

        // libération des ressources
        close_fifo(fd_client_master, "fd_client_master");
        close_fifo(fd_master_client, "fd_master_client");
    }
	
    return EXIT_SUCCESS;
}
