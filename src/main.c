/**
 * Auteurs : Abdelmoujib Benamara, David Ekchajzer, Mathieu Ridet
 *
 * NB : - Certaines librairies incluent ici ne fonctionnent pas sur Windows.
 *		- Commande pour lancer le programme : "gcc main.c -o nomExecutable -lpthread"
 *		  (Attention : ne pas oublier d'incluse le parametre "-lpthread" !)
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
#define TAILLE_MAX_REP_PROCESS_TRAITEMENT 25
#define TAILLE_MAX_FICHIER 1024

pthread_t thread1, thread2, thread3;

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Descripteurs de fichier (fd1 pour fichier 1 et fd2 pour fichier 2)
int fd1, fd2;

void *producteur_routine(void *i) {
	pthread_t tid = pthread_self();
	char bufFichiers[TAILLE_MAX_FICHIER];
	int k = 0;
	char c;

	/*
	while(c != '\n' && c != '\0') {
		c = 0;
		if(pthread_equal(thread2, tid)) {
			printf("&&&&&& producteur 1\n");
			// On est dans le premier thread consommateur, s'occupe du fichier 1
			if(read(fd1, &c, 1) == -1) {
				perror("Read dans le fichier 1 KO");
				exit(EXIT_FAILURE);
			}
		} else if(pthread_equal(thread3, tid)) {
			printf("&&&&&& producteur 2\n");
			// On est dans le deuxieme thread consommateur, s'occupe du fichier 2
			if(read(fd2, &c, 1) == -1) {
				perror("Read dans le fichier 1 KO");
				exit(EXIT_FAILURE);
			}
		}
		bufFichiers[k++] = c;
	}

	// Affichage du contenu des fichiers
	int numFichier;
	if(pthread_equal(thread2, tid)) numFichier = 1;
	else numFichier = 2;
	printf("\nLe contenu du fichier %d est : %s\n", numFichier, bufFichiers);
	
	// Chaque thread producteur retourne son tampon remplie
	pthread_exit(bufFichiers);
	*/
}


/**
	TODO : voir fd !!!
**/
void *consommateur_routine(void *i) {
	void * rep_thread2, * rep_thread3;
	int res = 0;

	// On attend la fin d'execution de nos 2 threads producteurs
	if(pthread_join(thread2, rep_thread2) != 0) {
		perror("pthread_join thread 2");
		exit(EXIT_FAILURE);
	} 
	if(pthread_join(thread3, rep_thread3) != 0) {
		perror("pthread_join thread 3");
		exit(EXIT_FAILURE);
	}

	printf("**** rep_thread2 : %s\n", rep_thread2);
	printf("**** rep_thread3 : %s\n", rep_thread3);

	// Comparer le contenu des 2 fichiers
	int lu1, lu2;
	char c1, c2;
	do {
		printf(" $$$$$$$$$$$$$ \n");
		if((lu1 = read(rep_thread2, c1, 1)) == -1) {
			perror("Read rep_thread2");
			exit(EXIT_FAILURE);
		}
		if((lu2 = read(rep_thread3, c2, 1)) == -1) {
			perror("Read rep_thread3");
			exit(EXIT_FAILURE);
		}

		if(c1 != c2) {
			res = 1;
			break;
		}

		printf(" ############ \n");
	} while(lu1 > 0);
	
	printf("\nrep_thread2 : &%p& *****\n", &rep_thread2);
	printf("rep_thread3 : &%p& *****\n", &rep_thread3);
	printf("res dans routine consommateur : %d\n", res);

	// Si res = 1, fichiers différents. Si res = 0, fichiers égaux.
	pthread_exit(res);
}


int main(int argc, char** argv) {
	// pid1 = processus entrees/sorties, pid2 = processus de traitement
	pid_t pid1, pid2;

	// t1 = processus entrees/sorties, t2 = processus de traitement
	int t1[2], t2[2];
	pipe(t1);
	pipe(t2);

	// Sert de tampon pour les transmissions entre processus
	char buf[TAILLE_MAX_CHEMIN];
	int status1, status2;

	// On cree le processus de traitement
	if((pid2 = fork()) == -1) {
		perror("fork processus de traitement");
		exit(EXIT_FAILURE);
	} else if(pid2 == 0) {
		// On est dans le processus de traitement
		close(t2[0]);
		close(t1[1]);

		// On recupere le premier chemin dans le tube t1 (le read est bloquant donc sert de 'pause')
		if(read(t1[0], buf, TAILLE_MAX_CHEMIN) == -1) {
			perror("Recuperation chemin 1 KO");
			exit(EXIT_FAILURE);
		}
		// On ouvre le fichier 1
		if((fd1 = open(buf, O_RDONLY)) == -1) {
			perror("Open fichier 1 KO");
			exit(EXIT_FAILURE);
		}
		
		printf("\n############# 1 ################\n");
		// On recupere le deuxieme chemin dans le tube t1
		if(read(t1[0], buf, TAILLE_MAX_CHEMIN) == -1) {
			perror("Recuperation chemin 2 KO");
			exit(EXIT_FAILURE);
		}
		// On ouvre le fichier 2
		if((fd2 = open(buf, O_RDONLY)) == -1) {
			perror("Open fichier 2 KO");
			exit(EXIT_FAILURE);
		}

		printf("\n############# 2 ################\n");
		// On crée les threads pour faire les traitements (dans les routines)
		if(pthread_create(&thread1, NULL, consommateur_routine, 0) != 0) {
			printf("Creation thread 1 KO\n");
			exit(EXIT_FAILURE);
		} else if(pthread_create(&thread2, NULL, producteur_routine, 0) != 0) {
			printf("Creation thread 2 KO\n");
			exit(EXIT_FAILURE);
		} else if(pthread_create(&thread3, NULL, producteur_routine, 0) != 0) {
			printf("Creation thread 3 KO\n");
			exit(EXIT_FAILURE);
		}

		printf("\n############# 3 ################\n");

		printf("\n");
		void * res;

		// On recupere le resultat de la comparaison des fichiers :
		// Si egaux, on a 1, sinon on a 0
		if(pthread_join(thread1, res) != 0) {
			//erreur (appel systeme ???)
		}

		printf("\n");
		printf("\n ************** res 2 = %d\n\n", res);
		printf("\n");

		// On transmet le resultat au processus d'E/S
		if(write(t2[1], res, sizeof(int)) == -1) {
			perror("Write res final KO");
			exit(EXIT_FAILURE);
		}

		printf("apres write final\n");

		// Processus de traitement termine avec succes
		close(t2[1]);
		close(t1[0]);
		exit(EXIT_SUCCESS);
	} else {
		// On est dans le processus pere
		if((pid1 = fork()) == -1) {
			perror("Fork processus E/S KO");
			exit(EXIT_FAILURE);
		} else if(pid1 == 0){
			// On est dans le processus E/S
			close(t1[0]);
			close(t2[1]);

			// Chemin vers fichier 1
			printf("Please enter the path to the first file : \n");
			gets(buf);
			// On transmet le premier chemin au processus de traitement via le tube t1
			if(write(t1[1], buf, TAILLE_MAX_CHEMIN) == -1) {
				perror("Write chemin fichier 1 KO");
				exit(EXIT_FAILURE);
			}

			// Chemin vers fichier 2
			printf("Then enter the path to the second file : \n");
			gets(buf);
			// On transmet le deuxieme chemin au processus de traitement via le tube t1
			if(write(t1[1], buf, TAILLE_MAX_CHEMIN) == -1) {
				perror("Write chemin fichier 2 KO");
				exit(EXIT_FAILURE);
			}

			printf("\n ************** buf = %s\n\n", buf);

			// Lecture de la reponse du processus de traitement via le tube t2
			if(read(t2[0], buf, 1) == -1) {
				perror("Read retour du processus de traitement KO ");
				exit(EXIT_FAILURE);
			}
		
			printf("\n ************** buf final = %s\n\n", buf);

			// Transmission resultat a l'utilisateur (sur la sortie standard)
			if(buf == 0) printf("Les 2 fichiers sont égaux!\n");
			else if(buf == 1) printf("Les fichiers sont différents!\n");
			else {
				printf("Erreur : le buffer ne contient ni 0 ni 1\n");
				exit(EXIT_FAILURE);
			}

			// Processus d'entrees/sorties termine avec succes
			close(t1[1]);
			close(t2[0]);
			exit(EXIT_SUCCESS);
		}

		waitpid(pid2, &status2, 0);

		/*
		TODO : cleanup_handler (pour libérer les ressources à la terminaison des threads!)
		*/

	}

	return 0;
}
