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
#define TAILLE_LIGNE 100

pthread_t thread1, thread2, thread3;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// Descripteurs de fichier (fd1 pour fichier 1 et fd2 pour fichier 2)
int fd1, fd2;

// Tampons contenant les lignes de nos fichiers
int pipe_fichier1[2], pipe_fichier2[2];

/*
void *producteur_routine(void *i) {
	pthread_t tid = pthread_self();
	char bufFichiers[TAILLE_MAX_FICHIER];
	int k = 0;
	char c;
	printf("debut routine producteur\n");
	do {
		printf("**************\n");
		c = 0;
		if(pthread_equal(thread2, tid)) {
			printf("&&&&&& producteur 1\n");
			// On est dans le premier thread consommateur (s'occupe du fichier 1)
			if(read(fd1, &c, 1) == -1) {
				perror("Read dans le fichier 1 KO");
				exit(EXIT_FAILURE);
			}
		} else if(pthread_equal(thread3, tid)) {
			printf("&&&&&& producteur 2\n");
			// On est dans le deuxieme thread consommateur (s'occupe du fichier 2)
			if(read(fd2, &c, 1) == -1) {
				perror("Read dans le fichier 1 KO");
				exit(EXIT_FAILURE);
			}
		}
		bufFichiers[k++] = c;
	} while(c != '\n' && c != '\0');

	// Affichage du contenu des fichiers
	int numFichier;
	if(pthread_equal(thread2, tid)) numFichier = 1;
	else numFichier = 2;
	printf("Le contenu du fichier %d est : %s\n", numFichier, bufFichiers);
	
	// Chaque thread producteur retourne son tampon remplie
	pthread_exit(bufFichiers);
}
*/

/*
void *consommateur_routine(void *i) {
	void * rep_thread2, * rep_thread3;
	int res = 0;

	printf("debut routine conso\n");

	// On attend la fin d'execution de nos 2 threads producteurs
	if(pthread_join(thread2, rep_thread2) != 0) {
		perror("pthread_join thread 2");
		exit(EXIT_FAILURE);
	} 
	if(pthread_join(thread3, rep_thread3) != 0) {
		perror("pthread_join thread 3");
		exit(EXIT_FAILURE);
	}

	printf("**** rep_thread2 : %p\n", rep_thread2);
	printf("**** rep_thread3 : %p\n", rep_thread3);

	// Comparer le contenu des 2 fichiers
	int lu1, lu2;
	char c1, c2;
	do {
		printf(" $$$$$$$$$$$$$ \n");
		if((lu1 = read(rep_thread2, &c1, 1)) == -1) {
			perror("Read rep_thread2");
			exit(EXIT_FAILURE);
		}
		if((lu2 = read(rep_thread3, &c2, 1)) == -1) {
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
	pthread_exit(&res);
}
*/


void *producteur_routine(void *i) {
	pthread_t tid = pthread_self();

	
		if(pthread_equal(thread2, tid)) {
			// On est dans le premier thread consommateur (s'occupe du fichier 1)
			printf("(INFO) *** Producteur 1 ***\n");

			//close(pipe_fichier2[1]);
			//close(pipe_fichier2[0]);

			//pthread_mutex_lock(&m);
			char tampon_fichier1[TAILLE_LIGNE];
			int k1 = 0, char_lu1 = 0;
			char c1;
			do {
				do {
					if((char_lu1 = read(fd1, &c1, 1)) == -1) {
						perror("Read dans le fichier 1 KO");
						exit(EXIT_FAILURE);
					}
					tampon_fichier1[k1++] = c1;
				} while(c1 != '\n');
				printf("tampon prod 1 : %s\n", tampon_fichier1);			
				// On ecrit la ligne lue dans le tube 1
				if(write(pipe_fichier1[1], &tampon_fichier1, TAILLE_LIGNE) == -1) {
					perror("(ERROR-153) Write producteur 1");
					exit(EXIT_FAILURE);
				}
				if((char_lu1 = read(fd1, &c1, 1)) == -1) {
						perror("Read dans le fichier 1 KO");
						exit(EXIT_FAILURE);
				}
				k1 = 0;
				char tampon_fichier1[TAILLE_LIGNE];
				tampon_fichier1[k1++] = c1;
			} while(char_lu1 != 0);
			

			/*
			if(c == '\n') {
				printf("fin de ligne fichier 1\n");
				// On arrete le remplissage du buffer (on attend que la ligne soit lu par le consommateur avant de continuer)
				// => on attend tant que les tubes ne sont pas vides
				int t1_vide;
				char buf_tmp1[TAILLE_LIGNE];
				while(1) {
					if((t1_vide = read(pipe_fichier1[0], buf_tmp1, TAILLE_LIGNE)) == -1) {
						perror("read test pipe 1 vide");
						exit(EXIT_FAILURE);
					}
					if(t1_vide == 0) {
						printf("tube 1 vide ok on continue\n");
						break;
					}
				}
			}
			*/
			//pthread_mutex_unlock(&m);

		} else if(pthread_equal(thread3, tid)) {
			// On est dans le deuxieme thread consommateur (s'occupe du fichier 2)
			printf("(INFO) *** Producteur 2 ***\n");

			//close(pipe_fichier1[1]);
			//close(pipe_fichier1[0]);

			//pthread_mutex_lock(&m);

			char tampon_fichier2[TAILLE_LIGNE];
			int k2 = 0, char_lu2 = 0;
			char c2;
			do {
				do {
					if((char_lu2 = read(fd2, &c2, 1)) == -1) {
						perror("Read dans le fichier 2 KO");
						exit(EXIT_FAILURE);
					}
					tampon_fichier2[k2++] = c2;
				} while(c2 != '\n');
				printf("tampon prod 2 : %s\n", tampon_fichier2);			
				// On ecrit la ligne lue dans le tube 1
				if(write(pipe_fichier2[1], &tampon_fichier2, TAILLE_LIGNE) == -1) {
					perror("(ERROR-207) Write producteur 2");
					exit(EXIT_FAILURE);
				}
				if((char_lu2 = read(fd2, &c2, 1)) == -1) {
						perror("Read dans le fichier 2 KO");
						exit(EXIT_FAILURE);
				}
				k2 = 0;
				char tampon_fichier2[TAILLE_LIGNE];
				tampon_fichier2[k2++] = c2;
			} while(char_lu2 != 0);
			//pthread_mutex_unlock(&m);
		}

	// TODO : annonce la fin des écrivains (donc read non bloquant) ???
	close(pipe_fichier1);	
	close(pipe_fichier2);

	// Chaque thread producteur retourne son tampon remplie
	//pthread_exit(null);
}



void *consommateur_routine(void *i) {
	int res = 0;
	void* ret = NULL;
	//close(pipe_fichier1[1]);
	//close(pipe_fichier2[1]);

	printf("(INFO-137) *** Consommateur ***\n");

	// Comparer le contenu des 2 fichiers
	char ligne_fichier1[TAILLE_LIGNE], ligne_fichier2[TAILLE_LIGNE];
	int char_lu1 = 0, char_lu2 = 0;

	// Tant que les lignes des 2 fichiers ne contiennent pas le char de fin de fichier, on lit et on compare !
	while(1) {	
		if((char_lu1 = read(pipe_fichier1[0], ligne_fichier1, TAILLE_LIGNE)) == -1) {
			perror("Read ligne fichier 1");
			exit(EXIT_FAILURE);
		}
		if((char_lu2 = read(pipe_fichier2[0], ligne_fichier2, TAILLE_LIGNE)) == -1) {
			perror("Read ligne fichier 2");
			exit(EXIT_FAILURE);
		}

		// TO DO !!!
		if(char_lu1 == 0 || char_lu2 == 0) {
			printf("test EoF*********\n");
			res = 1;
			break;
		}

		if(strcmp(ligne_fichier1, ligne_fichier2) == 0) {
			// Lignes équivalentes, on continue
			res = 0;
		} else {
			printf("test *********\n");
			// Lignes différentes, on close tous les threads(TODO !!!)
			res = 1; //Pour le moment à la fin si res = 1 => fichiers diff
			break;
		}
		
		printf("\nligne_fichier1 : %s\n", ligne_fichier1);
		printf("ligne_fichier2 : %s\n", ligne_fichier2);
		//printf("res dans routine consommateur : %d\n", res);
	}

	// Si res = 1, fichiers différents. Si res = 0, fichiers égaux.
	pthread_exit(&res);
}



int main(int argc, char** argv) {
	// pid1 = processus entrees/sorties, pid2 = processus de traitement
	pid_t pid1, pid2;

	// t1 = processus entrees/sorties, t2 = processus de traitement
	int t1[2], t2[2];
	pipe(t1);
	pipe(t2);

	// tampons contenant les lignes de nos fichiers
	pipe(pipe_fichier1);
	pipe(pipe_fichier2);

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
		
		//printf("\n############# 1 ################\n");
		
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

		//printf("\n############# 2 ################\n");
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

		//printf("\n############# 3 ################\n");

		printf("\n");
		void * res;

		// On recupere le resultat de la comparaison des fichiers :
		// Si egaux, on a 1, sinon on a 0
		if(pthread_join(thread1, res) != 0) {
			perror("pthread_join thread 1");
			exit(EXIT_FAILURE);
		}

		printf("\n");
		//printf("\n ************** res 2 = %d\n\n", res);
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

			//printf("\n ************** buf = %s\n\n", buf);

			// Lecture de la reponse du processus de traitement via le tube t2
			if(read(t2[0], buf, 1) == -1) {
				perror("Read retour du processus de traitement KO ");
				exit(EXIT_FAILURE);
			}
		
			//printf("\n ************** buf final = %s\n\n", buf);

			// Transmission resultat a l'utilisateur (sur la sortie standard)
			if(buf == 0) printf("Les 2 fichiers sont égaux!\n");
			else if(*buf == 1) printf("Les fichiers sont différents!\n");
			else {
				printf("(ERROR-401) Resultat KO : Le buffer ne contient ni 0 ni 1\n");
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
