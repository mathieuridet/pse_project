/**
 * Auteurs : Abdelmoujib Benamara, David Ekchajzer, Mathieu Ridet
 *
 * NB : - Certaines librairies incluent ici ne fonctionnent pas sur Windows.
 *		- Commande pour lancer le programme : "gcc main.c -o nomExecutable -lpthread"
 *		  (Attention : ne pas oublier d'inclure le parametre "-lpthread" !)
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>			// Ne fonctionne pas sur Windows!
#include <fcntl.h>
#include <string.h>

#define TAILLE_MAX_CHEMIN 50
#define TAILLE_MAX_FICHIER 1024
#define TAILLE_LIGNE 1000

pthread_t thread1, thread2, thread3;

pthread_barrier_t barrier;

// Mutex déclaré global pour pouvoir être utilisé dans toutes les sections critiques (main/routines)
//pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// Descripteurs de fichier (fd1 pour fichier 1 et fd2 pour fichier 2)
int fd1, fd2;

// Tube utilise pour la transmission de la valeur de resultat final
int t_res[2];

// Tampons contenant les contenus des fichiers
char *tampon_fichier1[TAILLE_LIGNE], *tampon_fichier2[TAILLE_LIGNE];

/*
static void cleanup_handler(void *arg){
	printf("(INFO-35) *** GESTIONNAIRE DE NETTOYAGE ***");
	printf("free %p\n", arg);
	free(arg);
}
*/

void *producteur_routine(void* f) {
	printf("(INFO-49) *** Producteur ***\n");

	pthread_t tid = pthread_self();
	int returnBarrier;
	char *tamponRoutine;
	int fd;
	//int numThread = *((int *) f);
	char car[2];
	int i;

	printf("PROD : APRES VAR\n");

	/*
	if (numThread == 1){
		fd = fd1;
		tamponRoutine = tampon_fichier1;
	}else if(numThread == 2){
		fd = fd2;
		tamponRoutine = tampon_fichier2;
	}else{
		perror("(ERROR-P101) Numéro de thread inconnue");
		exit(EXIT_FAILURE);
	}*/
	if (pthread_equal(thread2, tid)){
		fd = fd1;
		tamponRoutine = tampon_fichier1;
	}else if(pthread_equal(thread3, tid)){
		fd = fd2;
		tamponRoutine = tampon_fichier2;
	}

	printf("PROD : APRES CHOIX PROD 1 OU 2\n");

	// Ensemble de signaux a considerer
	sigset_t sigs_to_catch;
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGUSR2);

	while(1) {
		printf("PROD : DEBUT WHILE(1)\n");

		i = 0;
		memset(tamponRoutine, 0, TAILLE_LIGNE);
		while(car[0] != '\n' && car[0] != '\0'){
			if (read(fd, car, 1) == -1){
				perror("(ERROR-P102) lecture d'une ligne KO");
				exit(EXIT_FAILURE);
			}
			printf("PROD : DANS 2e WHILE\n");
			
			tamponRoutine[i] = car[0];
			i++;
		}

		// Si un thread arrive ici, il attend le 2e producteur avant de pouvoir continuer
		if(pthread_barrier_wait(&barrier) != 0){
			printf("PROD : ENVOI SIGNAL POUR CONTINUER\n");
					
			pthread_kill(&thread1, SIGUSR1);
		}
		// Les producteurs ont fini de remplir les tampons avec une ligne chacun, on envoit un signal au consommateur
		// On attend que le consommateur compare les lignes (reprennent quand reçoivent SIGUSR2);
		printf("PROD : ATTENTE SIGNAL\n");
		sigwait(&sigs_to_catch, NULL);
		printf("PROD : SIGNAL RECU\n");

	}

}



void *consommateur_routine(void *i) {
	printf("(INFO-122) *** Consommateur ***\n");

	// Si fichier identifique on retournera 0, sinon 1
	int result = 0;
	// Ensemble de signaux a considerer
	sigset_t sigs_to_catch;
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGUSR1);

	printf("CONSO : APRES VAR\n");

	while(1) {
		// On attend que les producteurs ecrivent leur ligne (reprend quand on recoit SIGUSR1)	
		printf("CONSO : AVANT RECEPTION SIGNAL\n");
		printf("CONSO : tampon_fichier1 = %s\n", *tampon_fichier1);
		printf("CONSO : tampon_fichier2 = %s\n", *tampon_fichier2);
		sigwait(&sigs_to_catch, NULL);
		printf("CONSO : APRES RECEPTION SIGNAL\n");
		printf("CONSO : tampon_fichier1 = %s\n", *tampon_fichier1);
		printf("CONSO : tampon_fichier2 = %s\n", *tampon_fichier2);

		// Les 2 fichiers possedent encore des lignes, on les compare
		if(strcmp(*tampon_fichier1, *tampon_fichier2)) {
			// On envoie un signal aux producteurs pour les sortir de leur pause et passer a la ligne suivante
			pthread_kill(&thread2, SIGUSR2);
			pthread_kill(&thread3, SIGUSR2);
		} else if(strcmp(*tampon_fichier1, "") && strcmp(*tampon_fichier2, "")) {
			// Si les 2 fichiers sont vides, on s'arrete 
			// (les fichiers sont equivalents sinon on se serait deja arrete)
			pthread_cancel(thread2);
			pthread_cancel(thread3);
			break;
		} 
		else {
			// Lignes differentes
			printf("CONSO : LIGNES DIFFERENTES\n");
			result = 1;

			// On stoppe les threads producteurs
			pthread_cancel(thread2);
			pthread_cancel(thread3);

			break;
		}
	}

	// TODO : gestionnaire de nettoyage


	printf("CONSO : FIN\n");

	// On stoppe le consommateur
	pthread_exit(result);
}


int main(int argc, char** argv) {
	// pid1 = processus entrees/sorties, pid2 = processus de traitement
	pid_t pid1, pid2;

	// t1 = processus entrees/sorties, t2 = processus de traitement
	int t1[2], t2[2];
	pipe(t1);
	pipe(t2);

	pipe(t_res);

	// Sert de tampon pour les transmissions entre processus (des chemins des fichiers)
	char buf[TAILLE_MAX_CHEMIN];
	// Sert de tampon pour les transmissions entre processus (la valeur de retour du consommateur)
	char buf_res[2];


	// On cree le processus de traitement
	if((pid2 = fork()) == -1) {
		perror("(ERROR-195) Fork processus de traitement");
		exit(EXIT_FAILURE);
	} else if(pid2 == 0) {
		// ***** PROCESUS DE TRAITEMENT *****
		close(t2[0]);
		close(t1[1]);

		// On recupere le premier chemin dans le tube t1 (le read est bloquant donc sert de 'pause')
		if(read(t1[0], buf, TAILLE_MAX_CHEMIN) == -1) {
			perror("(ERROR-204) Recuperation chemin 1 KO");
			exit(EXIT_FAILURE);
		}
		// On ouvre le fichier 1
		if((fd1 = open(buf, O_RDONLY)) == -1) {
			perror("(ERROR-209) Open fichier 1 KO");
			exit(EXIT_FAILURE);
		}
				
		// On recupere le deuxieme chemin dans le tube t1
		if(read(t1[0], buf, TAILLE_MAX_CHEMIN) == -1) {
			perror("(ERROR-215) Recuperation chemin 2 KO");
			exit(EXIT_FAILURE);
		}
		// On ouvre le fichier 2
		if((fd2 = open(buf, O_RDONLY)) == -1) {
			perror("(ERROR-220) Open fichier 2 KO");
			exit(EXIT_FAILURE);
		}

		// On crée les threads pour faire les traitements (dans les routines)
		if(pthread_create(&thread1, NULL, consommateur_routine, 0) != 0) {
			printf("(ERROR-226) Creation thread 1 KO\n");
			exit(EXIT_FAILURE);
		} else if(pthread_create(&thread2, NULL, producteur_routine, 1) != 0) {
			printf("(ERROR-229) Creation thread 2 KO\n");
			exit(EXIT_FAILURE);
		} else if(pthread_create(&thread3, NULL, producteur_routine, 2) != 0) {
			printf("(ERROR-232) Creation thread 3 KO\n");
			exit(EXIT_FAILURE);
		}

		// On recupere le resultat de la comparaison des fichiers :
		// Si egaux status = 0, sinon status = 1
		void *status = -1;
		pthread_join(thread1, &status);
		// On transmet le resultat au processus d'E/S
		buf_res[0] = (int) status;
		if(write(t_res[1], buf_res, sizeof(buf_res)) == -1) {
			perror("(ERROR-245) Write res final KO");
			exit(EXIT_FAILURE);
		}

		// Processus de traitement termine avec succes
		//close(t2[1]);
		//close(t1[0]);
		
		exit(EXIT_SUCCESS);
	} else {
		// ***** PROCESSUS PERE *****
		/**
		 * Il n'y a rien à faire ici.
		 * On crée juste le processus d'E/S.
		**/

		if((pid1 = fork()) == -1) {
			perror("(ERROR-261) Fork processus E/S KO");
			exit(EXIT_FAILURE);
		} else if(pid1 == 0){
			// ***** PROCESSUS E/S *****
			close(t1[0]);
			close(t2[1]);
			close(t_res[1]);

			// Chemin vers fichier 1
			printf("Please enter the path to the first file : \n");
			gets(buf);
			// On transmet le premier chemin au processus de traitement via le tube t1
			if(write(t1[1], buf, TAILLE_MAX_CHEMIN) == -1) {
				perror("(ERROR-273) Write chemin fichier 1 KO");
				exit(EXIT_FAILURE);
			}

			// Chemin vers fichier 2
			printf("Then enter the path to the second file : \n");
			gets(buf);
			// On transmet le deuxieme chemin au processus de traitement via le tube t1
			if(write(t1[1], buf, TAILLE_MAX_CHEMIN) == -1) {
				perror("(ERROR-282) Write chemin fichier 2 KO");
				exit(EXIT_FAILURE);
			}

			// Lecture de la reponse du processus de traitement via le tube t_res
			if(read(t_res[0], buf_res, sizeof(buf_res)) == -1) {
				perror("(ERROR-288) Read retour du processus de traitement KO ");
				exit(EXIT_FAILURE);
			}
			// Transmission resultat a l'utilisateur (sur la sortie standard)
			if(buf_res[0] == 0) printf("(INFO) Les 2 fichiers sont identiques!\n");
			else if(buf_res[0] == 1) printf("(INFO) Les fichiers sont différents!\n");
			else {
				printf("(ERROR-296) Resultat KO : Le buffer contient une valeur de retour inconnue !\n");
				exit(EXIT_FAILURE);
			}

			// Processus d'entrees/sorties termine avec succes
			close(t1[1]);
			close(t2[0]);
			close(t_res[0]);
			exit(EXIT_SUCCESS);
		}

		// On attend la fin du processus de traitement
		int status = 100;
		waitpid(pid2, &status, 0);
		
		// On appelle le gestionnaire de nettoyage pour libérer les ressources utilisées.
		//pthread_cleanup_pop(1);

	}

	return 0;
}
