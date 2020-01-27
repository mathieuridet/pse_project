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
#include <pthread.h>
#include <fcntl.h>
#include <string.h>

#define TAILLE_MAX_CHEMIN 50
#define TAILLE_MAX_FICHIER 1024
#define TAILLE_LIGNE 1000
#define TAILLE_TUBES 2
#define NB_PRODUCTEURS 2

// Declaration de nos pthreads
pthread_t thread1, thread2, thread3;

// Declaration de la barriere utilisee dans la routine Producteur
pthread_barrier_t barrier;

// Descripteurs de fichier (fd1 pour fichier 1 et fd2 pour fichier 2)
int fd1, fd2, sig;

// pid1 = processus entrees/sorties, pid2 = processus de traitement, pid3 = processus de controle
pid_t pid1, pid2, pid3;

// Tube utilise pour la transmission de la valeur de resultat final
int t_res[TAILLE_TUBES];

// Tampons contenant les contenus des fichiers
char *tampon_fichier1[TAILLE_LIGNE], *tampon_fichier2[TAILLE_LIGNE];

// Gestionnaire de nettoyage
static void cleanup_handler(void *arg){
	printf("(INFO-35) *** GESTIONNAIRE DE NETTOYAGE ***");
	printf("free %p\n", arg);
	free(arg);
}

void *producteur_routine(void* arg) {
	pthread_t tid = pthread_self();
	int fd, i, retour_barrier;
	char c, *tamponRoutine;

	if (pthread_equal(thread2, tid)){
		fd = fd1;
		tamponRoutine = (char*) tampon_fichier1;
	}else if(pthread_equal(thread3, tid)){
		fd = fd2;
		tamponRoutine = (char*) tampon_fichier2;
	}

	// Ensemble de signaux a considerer
	sigset_t sigs_to_catch;
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGUSR2);

	while(1) {
		i = 0;
		memset(tamponRoutine, 0, TAILLE_LIGNE);
		do {
			if (read(fd, &c, 1) == -1){
				perror("(ERROR-P102) lecture d'une ligne KO");
				exit(EXIT_FAILURE);
			}
			tamponRoutine[i] = c;
			i++;
		} while(c != '\n' && c != '\0');

		// Si un thread arrive ici, il attend le 2e producteur avant de pouvoir continuer
		retour_barrier = pthread_barrier_wait(&barrier);

		if(retour_barrier != 0 && retour_barrier != PTHREAD_BARRIER_SERIAL_THREAD) {
			perror("pthread_barrier_wait");
			exit(EXIT_FAILURE);
		}
		else if(retour_barrier == PTHREAD_BARRIER_SERIAL_THREAD){
			// Un seul thread a cette valeur de retour, il envoit le signal au consommateur
			pthread_kill(thread1, SIGUSR1);
		}
		
		// Les producteurs ont fini de remplir les tampons avec une ligne chacun, on envoit un signal au consommateur
		// On attend que le consommateur compare les lignes (reprennent quand reçoivent SIGUSR2);
		if(sigwait(&sigs_to_catch, &sig) != 0) {
			perror("(ERROR) sigwait dans producteur");
			exit(EXIT_FAILURE);
		}
	}

	pthread_barrier_destroy(&barrier);

}



void *consommateur_routine(void *arg) {
	// Si fichier identifique on retournera 0, sinon num_ligne
	int result = -1, num_ligne = 1;
	// Ensemble de signaux a considerer
	sigset_t sigs_to_catch;
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGUSR1);

	while(1) {
		// On attend que les producteurs ecrivent leur ligne (reprend quand on recoit SIGUSR1)	
		if(sigwait(&sigs_to_catch, &sig) != 0) {
			perror("(ERROR) sigwait dans consommateur");
			exit(EXIT_FAILURE);
		}

		// Les 2 fichiers possedent encore des lignes, on les compare
		if(strcmp(*tampon_fichier1, "\n\0")!=0 && strcmp(*tampon_fichier1, *tampon_fichier2)==0) {
			// On met result a 0 = lignes identiques
			result = 0;			
			if(*tampon_fichier1[0]=='\0') {
				// Les fichiers sont vides a la base
				pthread_cancel(thread2);
				pthread_cancel(thread3);
				break;
			}
			num_ligne++;
			// On envoie un signal aux producteurs pour les sortir de leur pause et passer a la ligne suivante
			pthread_kill(thread2, SIGUSR2);
			pthread_kill(thread3, SIGUSR2);
		} else if(strcmp(*tampon_fichier1, "\n\0") == 0 && strcmp(*tampon_fichier2, "\n\0") == 0) {
			// Il n'y a plus de lignes à lire dans aucun des 2 fichiers, on s'arrete 
			// (les fichiers sont equivalents sinon on se serait deja arrete)
			pthread_cancel(thread2);
			pthread_cancel(thread3);
			break;
		} else {
			// Lignes differentes

			// On stoppe les threads producteurs
			pthread_cancel(thread2);
			pthread_cancel(thread3);
			// Le consommateur se termine en retournant le num de la ligne qui diffère
			pthread_exit(&num_ligne);
			break;
		}
	}

	// On stoppe le consommateur
	pthread_exit(&result);
}

/*	
	Gestionnaire a effectuer si un processus fils est arrete.
	Utile ici car si l'utilisateur se trompe dans un chemin de fichier, 
	le processus de traitement s'arrete. Ainsi, on tue le processus d'E/S.
 */
void sig_hand(int sig){
	kill(pid1, SIGKILL);
}


int main(int argc, char** argv) {
	// Initialisation de la barriere des threads producteurs
	pthread_barrier_init(&barrier,NULL,NB_PRODUCTEURS);

	// Stocker le PID du processus de controle
	pid3 = getpid();

	// Structure sigaction pour gerer SIGCHLD (pour gerer la mauvaise terminaision d'un des processus)
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sig_hand;
	sigaction(SIGCHLD, &sa, NULL);

	// Déclaration tubes : t1 = processus entrees/sorties, t2 = processus de traitement
	int t1[TAILLE_TUBES], t2[TAILLE_TUBES];
	// Initialisation de nos tubes
	pipe(t1);
	pipe(t2);
	pipe(t_res);

	// Sert de tampon pour les transmissions entre processus (des chemins des fichiers)
	char buf_reception_chemin[TAILLE_MAX_CHEMIN];
	char *buf_envoi_chemin = NULL;

	// Sert de tampon pour les transmissions entre processus (la valeur de retour du consommateur)
	char buf_res[2];

	// On cree un masque de signaux pour le processus principal
	// afin qu'il ne reagisse pas aux signaux SIGUSR1 & SIGUSR2
	sigset_t sigs_to_ignore;
	sigemptyset(&sigs_to_ignore);
	sigaddset(&sigs_to_ignore, SIGUSR1);
	sigaddset(&sigs_to_ignore, SIGUSR2);
	sigaddset(&sigs_to_ignore, SIGSEGV);
	sigprocmask(SIG_BLOCK, &sigs_to_ignore, NULL);

	// A partir d'ici, si un thread se termine, l'handler sera appele
	pthread_cleanup_push(cleanup_handler, NULL);

	// On cree le processus de traitement
	if((pid2 = fork()) == -1) {
		perror("(ERROR-195) Fork processus de traitement");
		exit(EXIT_FAILURE);
	} else if(pid2 == 0) {
		// ***** PROCESUS DE TRAITEMENT *****
		close(t2[0]);
		close(t1[1]);

		// On recupere le premier chemin dans le tube t1 (le read est bloquant donc sert de 'pause')
		if(read(t1[0], buf_reception_chemin, TAILLE_MAX_CHEMIN) == -1) {
			perror("(ERROR-204) Recuperation chemin 1");
			exit(EXIT_FAILURE);
		}

		printf("********** %s**", buf_reception_chemin);

		// On ouvre le fichier 1
		if((fd1 = open(buf_reception_chemin, O_RDONLY)) == -1) {
			perror("(ERROR-209) Open fichier 1");
			exit(EXIT_FAILURE);
		}
				
		// On recupere le deuxieme chemin dans le tube t1
		if(read(t1[0], buf_reception_chemin, TAILLE_MAX_CHEMIN) == -1) {
			perror("(ERROR-215) Recuperation chemin 2");
			exit(EXIT_FAILURE);
		}
		// On ouvre le fichier 2
		if((fd2 = open(buf_reception_chemin, O_RDONLY)) == -1) {
			perror("(ERROR-220) Open fichier 2");
			exit(EXIT_FAILURE);
		}

		// On crée les threads pour faire les traitements (dans les routines)
		if(pthread_create(&thread1, NULL, consommateur_routine, NULL) != 0) {
			printf("(ERROR-226) Creation thread 1\n");
			exit(EXIT_FAILURE);
		} else if(pthread_create(&thread2, NULL, producteur_routine, NULL) != 0) {
			printf("(ERROR-229) Creation thread 2\n");
			exit(EXIT_FAILURE);
		} else if(pthread_create(&thread3, NULL, producteur_routine, NULL) != 0) {
			printf("(ERROR-232) Creation thread 3\n");
			exit(EXIT_FAILURE);
		}


		// On recupere le resultat de la comparaison des fichiers :
		int * res_consommateur;
		pthread_join(thread1, (void**) &res_consommateur);

		// On transmet le resultat au processus d'E/S
		buf_res[0] = *res_consommateur;
		if(write(t_res[1], buf_res, sizeof(buf_res)) == -1) {
			perror("(ERROR-245) Write res final");
			exit(EXIT_FAILURE);
		}

		// On ferme les differents tubes dont on a plus besoin
		close(t2[1]);
		close(t1[0]);
		close(t_res[1]);

		// Processus de traitement termine avec succes		
		exit(EXIT_SUCCESS);
	} else {
		// ***** PROCESSUS PERE *****

		if((pid1 = fork()) == -1) {
			perror("(ERROR-261) Fork processus E/S");
			exit(EXIT_FAILURE);
		} else if(pid1 == 0){
			// ***** PROCESSUS E/S *****
			close(t1[0]);
			close(t2[1]);
			close(t_res[1]);

			// Chemin vers fichier 1
			printf("Please enter the path to the first file : \n");
			//gets(buf);

			// TODO : voir comment enlever \n a la fin de getline!
			size_t len = 0;
			getline(&buf_envoi_chemin, &len, stdin);

			// On transmet le premier chemin au processus de traitement via le tube t1
			if(write(t1[1], buf_envoi_chemin, TAILLE_MAX_CHEMIN) == -1) {
				perror("(ERROR-273) Write chemin fichier 1");
				exit(EXIT_FAILURE);
			}

			// Chemin vers fichier 2
			printf("Then enter the path to the second file : \n");
			//gets(buf);
			getline(&buf_envoi_chemin, &len, stdin);
		

			// On transmet le deuxieme chemin au processus de traitement via le tube t1
			if(write(t1[1], buf_envoi_chemin, TAILLE_MAX_CHEMIN) == -1) {
				perror("(ERROR-282) Write chemin fichier 2");
				exit(EXIT_FAILURE);
			}

			printf("\n");

			// Lecture de la reponse du processus de traitement via le tube t_res
			if(read(t_res[0], buf_res, sizeof(buf_res)) == -1) {
				perror("(ERROR-288) Read retour du processus de traitement");
				exit(EXIT_FAILURE);
			}
			// Transmission resultat a l'utilisateur (sur la sortie standard)
			if(buf_res[0] == 0) printf("(SUCCESS) Les 2 fichiers sont identiques!\n");
			else {
				printf("(FAILURE) Les fichiers sont différents!\n          La ligne %d diffère.\n", buf_res[0]);
				exit(EXIT_FAILURE);
			}
			printf("\n");

			// Processus d'entrees/sorties termine avec succes
			close(t1[1]);
			close(t2[0]);
			close(t_res[0]);
			exit(EXIT_SUCCESS);
		}
		
		// On attend la fin du processus de traitement
		waitpid(pid2, NULL, 0);

	}

	// On libere les differentes ressources
	//free(*buf_res);	// TODO : voir pq "Warning"!
	//free(*buf);		// TODO : voir pq fonctionne pas (segmentation fault)
	free(*tampon_fichier1);
	free(*tampon_fichier2);
	pthread_barrier_destroy(&barrier);

	// Si des threads s'arrete avant ici, le gestionnaire de nettoyage sera appele
	pthread_cleanup_pop(1);

	return 0;
}
