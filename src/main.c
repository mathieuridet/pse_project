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

// Mutex déclaré global pour pouvoir être utilisé dans toutes les sections critiques (main/routines)
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// Descripteurs de fichier (fd1 pour fichier 1 et fd2 pour fichier 2)
int fd1, fd2;

// Tampons contenant les lignes de nos fichiers
int pipe_fichier1[2], pipe_fichier2[2], t_res[2];

/*
static void cleanup_handler(void *arg){
	printf("(INFO-35) *** GESTIONNAIRE DE NETTOYAGE ***");
	printf("free %p\n", arg);
	free(arg);
}
*/

void *producteur_routine(void *i) {
	pthread_t tid = pthread_self();


		if(pthread_equal(thread2, tid)) {
			// On est dans le premier thread consommateur (s'occupe du fichier 1)
			printf("(INFO-47) *** Producteur 1 ***\n");

			char tampon_fichier1[TAILLE_LIGNE];
			int k1 = 0, char_lu1 = -1;
			char c1;
			while(1) {
				do {
					if((char_lu1 = read(fd1, &c1, 1)) == -1) {
						perror("(ERROR-55) Read dans le fichier 1 KO");
						exit(EXIT_FAILURE);
					}
					tampon_fichier1[k1++] = c1;
				} while(c1 != '\n');
				// On ecrit la ligne lue dans le tube 1
				if(write(pipe_fichier1[1], &tampon_fichier1, TAILLE_LIGNE) == -1) {
					perror("(ERROR-62) Write producteur 1");
					exit(EXIT_FAILURE);
				}
				if((char_lu1 = read(fd1, &c1, 1)) == -1) {
						perror("(ERROR-66) Read dans le fichier 1 KO");
						exit(EXIT_FAILURE);
				}
				if(char_lu1 == 0) {
					if(write(pipe_fichier1[1], "\0", sizeof(char)) == -1) {
						perror("(ERROR-71) Write char fin de fichier producteur 1");
						exit(EXIT_FAILURE);
					}
					break;
				}
				k1 = 0;
				tampon_fichier1[k1++] = c1;
			}
		} else if(pthread_equal(thread3, tid)) {
			// On est dans le deuxieme thread consommateur (s'occupe du fichier 2)
			printf("(INFO-81) *** Producteur 2 ***\n");

			char tampon_fichier2[TAILLE_LIGNE];
			int k2 = 0, char_lu2 = -1;
			char c2;
			while(1) {
				do {
					if((char_lu2 = read(fd2, &c2, 1)) == -1) {
						perror("(ERROR-89) Read dans le fichier 2 KO");
						exit(EXIT_FAILURE);
					}
					tampon_fichier2[k2++] = c2;
				} while(c2 != '\n');
				// On ecrit la ligne lue dans le tube 1
				if(write(pipe_fichier2[1], &tampon_fichier2, TAILLE_LIGNE) == -1) {
					perror("(ERROR-96) Write producteur 2");
					exit(EXIT_FAILURE);
				}
				if((char_lu2 = read(fd2, &c2, 1)) == -1) {
						perror("(ERROR-100) Read dans le fichier 2 KO");
						exit(EXIT_FAILURE);
				}
				if(char_lu2 == 0) {
					if(write(pipe_fichier2[1], "\0", sizeof(char)) == -1) {
						perror("(ERROR-105) Write char fin de fichier producteur 2");
						exit(EXIT_FAILURE);
					}
					break;
				} 
				k2 = 0;
				tampon_fichier2[k2++] = c2;
			}
		}
}



void *consommateur_routine(void *i) {
	printf("(INFO-122) *** Consommateur ***\n");

	// Comparer le contenu des 2 fichiers
	char ligne_fichier1[TAILLE_LIGNE], ligne_fichier2[TAILLE_LIGNE], res[2];
	int char_lu1 = -1, char_lu2 = -1;
	char c1, c2;

/*
	pthread_cleanup_push(cleanup_handler, (void*) ligne_fichier1);
	pthread_cleanup_push(cleanup_handler, (void*) ligne_fichier2);
	pthread_cleanup_push(cleanup_handler, (void*) res);
*/

	// Tant que les lignes des 2 fichiers ne contiennent pas le char de fin de fichier, on lit et on compare !
	while(1) {	
		if((char_lu1 = read(pipe_fichier1[0], ligne_fichier1, TAILLE_LIGNE)) == -1) {
			perror("(ERROR-132) Read ligne fichier 1");
			exit(EXIT_FAILURE);
		}
		if((char_lu2 = read(pipe_fichier2[0], ligne_fichier2, TAILLE_LIGNE)) == -1) {
			perror("(ERROR-136) Read ligne fichier 2");
			exit(EXIT_FAILURE);
		}

		if((ligne_fichier1[0] == '\0' && ligne_fichier2[0] != '\0')
			|| (ligne_fichier1[0] != '\0' && ligne_fichier2[0] == '\0')) {
			// Lignes différentes :

			// On stoppe les threads producteurs
			pthread_cancel(thread2);
			pthread_cancel(thread3);

			// On arrete aussi le thread consommateur en renvoyant 1 (la valeur de retour d'échec)
			//pthread_exit((void*) 1);
			
			res[0] = '1';

			break;
		} else if(ligne_fichier1[0] == '\0' && ligne_fichier2[0] == '\0'){
			break;
		}

		if(strcmp(ligne_fichier1, ligne_fichier2) == 0) {
			// Lignes équivalentes, on continue
			res[0] = '0';
		} else {
			// Lignes différentes :

			// On stoppe les threads producteurs
			pthread_cancel(thread2);
			pthread_cancel(thread3);

			// TODO
			// On arrete aussi le thread consommateur en renvoyant 1 (la valeur de retour d'échec)
			//pthread_exit((void*) 1);
			
			res[0] = '1';

			break;
		}
	}

	// On ferme les 2 tubes en lecture (car l'activité de lecture est terminée).
	close(pipe_fichier1[0]);
	close(pipe_fichier2[0]);

	// Si res = 1, fichiers différents. Si res = 0, fichiers égaux.
	if(write(t_res[1], res, sizeof(int)) == -1) {
		perror("(ERROR-164) Write res au processus de traitement");
	}

	// On appelle le gestionnaire de nettoyage pour libérer les ressources utilisées.
	//pthread_cleanup_pop(1);
}


int main(int argc, char** argv) {
	// pid1 = processus entrees/sorties, pid2 = processus de traitement
	pid_t pid1, pid2;

	// t1 = processus entrees/sorties, t2 = processus de traitement
	int t1[2], t2[2];
	pipe(t1);
	pipe(t2);

	// Tubes utilisé pour la transmission des lignes de nos fichiers.
	// Ils sont déclarés en global pour être utilisés dans les routines.
	pipe(pipe_fichier1);
	pipe(pipe_fichier2);

	// Tube utilisé pour la transmission de la valeur de retour finale.
	pipe(t_res);

	// Sert de tampon pour les transmissions entre processus (des chemins des fichiers)
	char buf[TAILLE_MAX_CHEMIN];
	// Sert de tampon pour les transmissions entre processus (la valeur de retour du consommateur)
	char buf_res[2];

/*
	pthread_cleanup_push(cleanup_handler, (void*) buf);
	pthread_cleanup_push(cleanup_handler, (void*) buf_res);
	pthread_cleanup_push(cleanup_handler, (void*) t1);
	pthread_cleanup_push(cleanup_handler, (void*) t2);
	pthread_cleanup_push(cleanup_handler, (void*) pid1);
	pthread_cleanup_push(cleanup_handler, (void*) pid2);
	pthread_cleanup_push(cleanup_handler, (void*) t_res);
*/

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
		} else if(pthread_create(&thread2, NULL, producteur_routine, 0) != 0) {
			printf("(ERROR-229) Creation thread 2 KO\n");
			exit(EXIT_FAILURE);
		} else if(pthread_create(&thread3, NULL, producteur_routine, 0) != 0) {
			printf("(ERROR-232) Creation thread 3 KO\n");
			exit(EXIT_FAILURE);
		}

		// On recupere le resultat de la comparaison des fichiers :
		// Si egaux buf_res = 1, sinon buf_res = 0
		if(read(t_res[0], buf_res, sizeof(buf_res)) == -1) {
			perror("(ERROR-239) Read res dans processus de traitement\n");
			exit(EXIT_FAILURE);
		}

		// On transmet le resultat au processus d'E/S
		if(write(t2[1], buf_res, sizeof(buf_res)) == -1) {
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

			// Lecture de la reponse du processus de traitement via le tube t2
			if(read(t2[0], buf_res, sizeof(buf_res)) == -1) {
				perror("(ERROR-288) Read retour du processus de traitement KO ");
				exit(EXIT_FAILURE);
			}
		
			// Transmission resultat a l'utilisateur (sur la sortie standard)
			if(buf_res[0] == '0') printf("(INFO) Les 2 fichiers sont identiques!\n");
			else if(buf_res[0] == '1') printf("(INFO) Les fichiers sont différents!\n");
			else {
				printf("(ERROR-296) Resultat KO : Le buffer ne contient ni 0 ni 1\n");
				exit(EXIT_FAILURE);
			}

			// Processus d'entrees/sorties termine avec succes
			close(t1[1]);
			close(t2[0]);
			exit(EXIT_SUCCESS);
		}

		// On attend la fin du processus de traitement
		waitpid(pid2, NULL, 0);

		// On appelle le gestionnaire de nettoyage pour libérer les ressources utilisées.
		//pthread_cleanup_pop(1);

	}

	return 0;
}
