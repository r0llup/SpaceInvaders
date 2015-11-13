/*
	SpaceInvaders.c
	Lecler Quentin
	Blanpain Olivier
*/

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "Ecran.h"
#include "EcranX.h"
#include "Grille.h"

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

#define KEY_SPACE       32
#define KEY_DOWN        50
#define KEY_UP          51
#define KEY_LEFT        52
#define KEY_RIGHT       53
#define KEY_W			119
#define KEY_X			120
#define KEY_S			115
#define NB_LIGNE        22
#define NB_COLONNE 		18
#define VIDE             0
#define CANON1           1
#define CANON2           2
#define MISSILE          3
#define BOUCLIER1        4
#define BOUCLIER2        5
#define ALIEN            6
#define BOMBE            7
#define VAISSEAU_AMIRAL  8
#define GAUCHE          10
#define DROITE          11

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

typedef struct s_missile
{
	int colonne;
	int ligne;
	int joueur;
	pthread_t tid;
	struct s_missile *suivant;
} S_MISSILE;

typedef struct s_bombe
{
	int colonne;
	int ligne;
	pthread_t tid;
	struct s_bombe *suivant;
} S_BOMBE;

typedef struct s_canon
{
	int colonne;
	int fireOn; // temporisation sur le tir de missiles
	int joueur;
} S_CANON;

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

struct termios Term, SaveTerm;
struct sigaction Action;
int tab[NB_LIGNE][NB_COLONNE];
int lh = 1; // ligne haut
int lb = 9; // ligne bas
int cg = 0; // colonne gauche
int cd = 12; // colonne droite
int nbAliens = 35;
int level = 1;
int score1 = 0;
int score2 = 0;
int nbVies1 = 3;
int nbVies2 = 3;
int enVie1 = 0;
int enVie2 = 0;
int terminerVaisseauAmiral = 0;
int nbJoueurs = 1;
pthread_t threadCanon1;
pthread_t threadCanon2;
pthread_t threadClavier;
pthread_t threadMissile;
pthread_t threadTimeOut;
pthread_t threadInvaders;
pthread_t threadFlotteAliens;
pthread_t threadVaisseauAmiral;
pthread_t threadScore;
pthread_t threadBombe;
pthread_mutex_t mutexGrille;
pthread_mutex_t mutexListeMissiles;
pthread_mutex_t mutexFlotteAliens;
pthread_mutex_t mutexScore;
pthread_mutex_t mutexListeBombes;
pthread_mutex_t mutexVies;
pthread_cond_t condScore;
pthread_cond_t condVies;
pthread_cond_t condFlotteAliens;
pthread_key_t cleCanon;
S_MISSILE *pListeMissiles = NULL;
S_BOMBE *pListeBombes = NULL;

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

int ReadChar();
int SortieIoctl(int);
void InitialiseGrille();
void InitialiseBoucliers();
void *fctThreadCanon(void *t);
void *fctThreadClavier(void *t);
void *fctThreadMissile(void *t);
void *fctThreadMissileFin(void *t);
void *fctThreadTimeOut(void *t);
void *fctThreadInvaders(void *t);
void *fctThreadFlotteAliens(void *t);
void *fctThreadScore(void *t);
void *fctThreadBombe(void *t);
void *fctThreadBombeFin(void *t);
void *fctThreadVaisseauAmiral(void *t);
void HandlerSigUsr1(int Sig);
void HandlerSigUsr2(int Sig);
void HandlerSigHup(int Sig);
void HandlerSigInt(int Sig);
void HandlerSigQuit(int Sig);
void HandlerSigChld(int Sig);
void insereMissile(S_MISSILE *pm);
pthread_t getTidMissile(int l, int c);
S_MISSILE *getMissile(int l, int c);
S_MISSILE *retireMissile(S_MISSILE *pm);
timespec_t sleepThread(int ns);
void Verification_Toute_Ligne_Colonne_Tuee(int l, int c);
int DeplacerFlotte(int l, int newL, int maxL, int newMaxL, \
	int c, int newC, int maxC, int newMaxC);
void SupprimerFlotte(int l, int maxL, int c, int maxC);
void DessinerFlotte(int l,int maxL, int c, int maxC);
void insereBombe(S_BOMBE *pb);
pthread_t getTidBombe(int l, int c);
S_BOMBE *getBombe(int l, int c);
S_BOMBE *retireBombe(S_BOMBE *pb);
void pickUpAlien(int *l, int *c);
void freeSpecificZone(void *p);

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

int main(int argc, char *argv[])
{
	sigset_t mask;
    FILE *hfErr;
	S_CANON *structCanon1;
	S_CANON *structCanon2;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	srand((unsigned) time(NULL));

    if((hfErr = fopen("Trace.log", "w")) == (FILE *) NULL)
    {
	   Trace("(MAIN) Err. de fopen");
	   return 1;
    }

    if(dup2(fileno(hfErr), 2) == -1)
    {
	   Trace("(MAIN) Err. de dup2");
	   return 1;
    }

    pthread_mutex_init(&mutexGrille, NULL);
    pthread_mutex_init(&mutexListeMissiles, NULL);
	pthread_mutex_init(&mutexFlotteAliens, NULL);
	pthread_mutex_init(&mutexScore, NULL);
	pthread_mutex_init(&mutexListeBombes, NULL);
	pthread_mutex_init(&mutexVies, NULL);
	pthread_cond_init(&condScore, NULL);
	pthread_cond_init(&condVies, NULL);
	pthread_cond_init(&condFlotteAliens, NULL);
	pthread_key_create(&cleCanon, freeSpecificZone);

	do
	{
		printf("Veuillez encoder le nombre de joueur(s) : ");
		scanf("%d", &nbJoueurs);
	} while((nbJoueurs <= 0) || (nbJoueurs > 2));

    Trace("Ouverture de la grille de jeu ...");

    // GRILLE_ON affiche la grille
    if(OuvrirGrille(NB_LIGNE, NB_COLONNE, 25, "Space Invaders", GRILLE_OFF) == -1)
    {
	   Trace("(MAIN) Err. de OuvrirGrille");
	   return 1;
    }

	pthread_mutex_lock(&mutexGrille);
    InitialiseGrille();
	pthread_mutex_unlock(&mutexGrille);

    // lancement du thread clavier
    if((errno = pthread_create(&threadClavier, NULL, fctThreadClavier, NULL)))
    {
    	Trace("(MAIN) Err. de pthread_create");
    	return 1;
    }

	// lancement du thread score
	if((errno = pthread_create(&threadScore, NULL, fctThreadScore, NULL)))
	{
		Trace("(MAIN) Err. de pthread_create");
		return 1;
	}

    // lancement du thread invaders
    if((errno = pthread_create(&threadInvaders, NULL, fctThreadInvaders, NULL)))
    {
    	Trace("(MAIN) Err. de pthread_create");
    	return 1;
    }

	// lancement du thread vaisseau amiral
	if((errno = pthread_create(&threadVaisseauAmiral, NULL, \
		fctThreadVaisseauAmiral, NULL)))
	{
    	Trace("(MAIN) Err. de pthread_create");
    	return 1;
    }

	if(nbJoueurs == 1)
	{
		pthread_mutex_lock(&mutexVies);

		while(nbVies1 > 0)
		{
			structCanon1 = (S_CANON *) malloc(sizeof(S_CANON));
			structCanon1->colonne = 8;
			structCanon1->joueur = 1;

			// lancement du thread canon du joueur 1
    		if((errno = pthread_create(&threadCanon1, NULL, \
    			fctThreadCanon, structCanon1)))
 			{
				Trace("(MAIN) Err. de pthread_create");
	  			return 1;
   			}

			pthread_cond_wait(&condVies, &mutexVies);
		}

		pthread_mutex_unlock(&mutexVies);
	}

	else
	{
		pthread_mutex_lock(&mutexVies);

		while((nbVies1 > 0) || (nbVies2 > 0))
		{
			if(!enVie1)
			{
				structCanon1 = (S_CANON *) malloc(sizeof(S_CANON));
				structCanon1->colonne = 8;
				structCanon1->joueur = 1;

				// lancement du thread canon du joueur 1
    			if((errno = pthread_create(&threadCanon1, NULL, \
    				fctThreadCanon, structCanon1)))
 				{	
					Trace("(MAIN) Err. de pthread_create");
	  				return 1;
   				}

				enVie1 = 1;
			}

			if(!enVie2)
			{
				structCanon2 = (S_CANON *) malloc(sizeof(S_CANON));
				structCanon2->colonne = 16;
				structCanon2->joueur = 2;

				// lancement du thread canon du joueur 2
    			if((errno = pthread_create(&threadCanon2, NULL, \
    				fctThreadCanon, structCanon2)))
 				{	
					Trace("(MAIN) Err. de pthread_create");
	  				return 1;
   				}

				enVie2 = 1;
			}

			pthread_cond_wait(&condVies, &mutexVies);
		}

		pthread_mutex_unlock(&mutexVies);
	}

    FermerGrille();

    if(tcsetattr(0, TCSANOW, &SaveTerm) == -1)
    {
    	return 1;
    }

	fclose(hfErr);
	pthread_key_delete(cleCanon);
	return 0;
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void InitialiseGrille()
{
    for(int i=0; i < NB_LIGNE; i++)
	{
   		for(int j=0; j < NB_COLONNE; j++)
    	{
        	tab[i][j] = VIDE;
        	DessineVide(i, j);
    	}
	}

	InitialiseBoucliers();
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void InitialiseBoucliers()
{
    int l = NB_LIGNE - 2;

    for(int i=1; i <= 2; i++)
    {
        tab[l][3] = BOUCLIER1; DessineBouclier(l, 3, 1);
        tab[l][4] = BOUCLIER1; DessineBouclier(l, 4, 1);
        tab[l][8] = BOUCLIER1; DessineBouclier(l, 8, 1);
        tab[l][9] = BOUCLIER1; DessineBouclier(l, 9, 1);
        tab[l][13] = BOUCLIER1; DessineBouclier(l, 13, 1);
        tab[l][14] = BOUCLIER1; DessineBouclier(l, 14, 1);
        l--;
    }
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

int ReadChar()
{
	char ChLu;

	if(tcgetattr(0, &Term) == -1)
	{
	   return -1;
	}

	SaveTerm = Term;
	Term.c_lflag &= ~(ICANON | ECHO | ISIG);
	Term.c_cc[VMIN] = 1;

	if(tcsetattr(0, TCSANOW, &Term) == -1)
	{
	   return -1;
	}

	fflush(stdin);

	if(read(0, &ChLu, 1) != 1)
	{
	   return SortieIoctl(-1);
	}

	if(ChLu == Term.c_cc[VINTR])
	{
	   return SortieIoctl(3);
	}

	if(ChLu == '\033')
	{
		if(read(0, &ChLu, 1) == 1)
    	{
			if ((char) ChLu == '[')
			{
            	if(read(0, &ChLu, 1) != 1)
				{
					return SortieIoctl(-1); 
				}

            	if((char) ChLu == 'A')
				{
					return SortieIoctl(KEY_UP);
				}

            	if((char) ChLu == 'B')
				{
					return SortieIoctl(KEY_DOWN);
				}

            	if((char) ChLu == 'C')
				{
					return SortieIoctl(KEY_RIGHT);
				}

            	if((char) ChLu == 'D')
				{
					return SortieIoctl(KEY_LEFT);
				}

            	return SortieIoctl(-1);
			}
		}
	}

	return SortieIoctl(ChLu);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

int SortieIoctl(int Code)
{
	if(tcsetattr(0, TCSANOW, &SaveTerm) == -1)
	{
		return -1;
    }

	return Code;
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadCanon(void *t)
{
	sigset_t mask;
	S_CANON *structCanon = (S_CANON *) t;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	sigemptyset(&Action.sa_mask);
	Action.sa_flags = 0;
	Action.sa_handler = HandlerSigUsr1;

	if(sigaction(SIGUSR1, &Action, NULL) == -1) 
   	{
		Trace("(THREAD_CANON) Err. de sigaction");
     	exit(1);
   	}

   	Action.sa_handler = HandlerSigUsr2;

	if(sigaction(SIGUSR2, &Action, NULL) == -1) 
   	{
		Trace("(THREAD_CANON) Err. de sigaction");
     	exit(1);
   	}

   	Action.sa_handler = HandlerSigHup;

   	if(sigaction(SIGHUP, &Action, NULL) == -1)
   	{
   		Trace("(THREAD_CANON) Err. de sigaction");
   		exit(1);
   	}

	Action.sa_handler = HandlerSigQuit;

   	if(sigaction(SIGQUIT, &Action, NULL) == -1)
   	{
   		Trace("(THREAD_CANON) Err. de sigaction");
   		exit(1);
   	}

	pthread_setspecific(cleCanon, structCanon);
	structCanon->fireOn = 1;
	pthread_mutex_lock(&mutexGrille);

	if(tab[NB_LIGNE-1][structCanon->colonne] == VIDE)
	{
		tab[NB_LIGNE-1][structCanon->colonne] = structCanon->joueur;
		DessineCanon(NB_LIGNE-1, structCanon->colonne, structCanon->joueur);
	}

	pthread_mutex_unlock(&mutexGrille);

	while(1)
	{
		pause();
	}

	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadClavier(void *t)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);

   	while(1)
	{
		switch(ReadChar())
		{
			case KEY_LEFT:
				pthread_kill(threadCanon1, SIGUSR1);
				break;
			case KEY_W:
				pthread_kill(threadCanon2, SIGUSR1);
				break;
			case KEY_RIGHT:
				pthread_kill(threadCanon1, SIGUSR2);
				break;
			case KEY_X:
				pthread_kill(threadCanon2, SIGUSR2);
				break;
			case KEY_SPACE:
				pthread_kill(threadCanon1, SIGHUP);
				break;
			case KEY_S:
				pthread_kill(threadCanon2, SIGHUP);
				break;
			case 3:
				kill(getpid(), SIGKILL);
				break;
		}
	}

	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadMissile(void *t)
{
	sigset_t mask;
    S_MISSILE *pMissile = (S_MISSILE *) t;
    int fini = 0;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	sigemptyset(&Action.sa_mask);
	Action.sa_flags = 0;
	Action.sa_handler = HandlerSigInt;

	if(sigaction(SIGINT, &Action, NULL) == -1) 
   	{
		Trace("(THREAD_MISSILE) Err. de sigaction");
		exit(1);
   	}

	pMissile->tid = pthread_self();
	pthread_cleanup_push(fctThreadMissileFin, pMissile);
	pthread_mutex_lock(&mutexListeMissiles);
	insereMissile(pMissile);
	pthread_mutex_unlock(&mutexListeMissiles);

	while((!fini) && (pMissile->ligne >= 0))
	{
		Trace("(THREAD_MISSILE) Ligne %d Colonne %d", \
			pMissile->ligne, pMissile->colonne);
		pthread_mutex_lock(&mutexGrille);

		switch(tab[pMissile->ligne][pMissile->colonne])
		{
			case VIDE:
				if(tab[pMissile->ligne+1][pMissile->colonne] == MISSILE)
				{
					tab[pMissile->ligne+1][pMissile->colonne] = VIDE;
					DessineVide(pMissile->ligne+1, pMissile->colonne);
				}

				tab[pMissile->ligne][pMissile->colonne] = MISSILE;
				DessineMissile(pMissile->ligne, pMissile->colonne);
				pMissile->ligne--;
				break;
			case BOUCLIER1:
				if(tab[pMissile->ligne+1][pMissile->colonne] == MISSILE)
				{
					pthread_mutex_unlock(&mutexGrille);
					sleepThread(30000000); // affichage du missile
					pthread_mutex_lock(&mutexGrille);
					tab[pMissile->ligne+1][pMissile->colonne] = VIDE;
					DessineVide(pMissile->ligne+1, pMissile->colonne);
				}

				tab[pMissile->ligne][pMissile->colonne] = BOUCLIER2;
				DessineBouclier(pMissile->ligne, pMissile->colonne, 2);
				fini = 1;
				break;
			case BOUCLIER2:
				if(tab[pMissile->ligne+1][pMissile->colonne] == MISSILE)
				{
					pthread_mutex_unlock(&mutexGrille);
					sleepThread(30000000); // affichage du missile
					pthread_mutex_lock(&mutexGrille);
					tab[pMissile->ligne+1][pMissile->colonne] = VIDE;
					DessineVide(pMissile->ligne+1, pMissile->colonne);
				}

				tab[pMissile->ligne][pMissile->colonne] = VIDE;
				DessineVide(pMissile->ligne, pMissile->colonne);
				fini = 1;
				break;
			case ALIEN:
				if(tab[pMissile->ligne+1][pMissile->colonne] == MISSILE)
				{
					tab[pMissile->ligne+1][pMissile->colonne] = VIDE;
					DessineVide(pMissile->ligne+1,pMissile->colonne);
				}

				tab[pMissile->ligne][pMissile->colonne] = VIDE;
				DessineVide(pMissile->ligne, pMissile->colonne);
				pthread_mutex_lock(&mutexFlotteAliens);
				nbAliens--;

				if(pMissile->ligne == lh || pMissile->ligne == lb || \
					pMissile->colonne == cg || pMissile->colonne == cd)
				{
					Verification_Toute_Ligne_Colonne_Tuee(lh, cg); 
					Verification_Toute_Ligne_Colonne_Tuee(lb, cd); 
				}

				pthread_mutex_unlock(&mutexFlotteAliens);
				pthread_mutex_lock(&mutexScore);

				if(pMissile->joueur == 1)
				{
					score1++;
				}

				else
				{
					score2++;
				}

				pthread_mutex_unlock(&mutexScore);
				pthread_cond_signal(&condScore);
				pthread_cond_signal(&condFlotteAliens);
				fini = 1;
				break;
			case VAISSEAU_AMIRAL:
				if(tab[pMissile->ligne+1][pMissile->colonne] == MISSILE)
				{
					tab[pMissile->ligne+1][pMissile->colonne] = VIDE;
					DessineVide(pMissile->ligne+1,pMissile->colonne);
				}

				tab[pMissile->ligne][pMissile->colonne] = VIDE;
				DessineVide(pMissile->ligne, pMissile->colonne);
				pthread_mutex_lock(&mutexScore);

				if(pMissile->joueur == 1)
				{
					score1 += 10;
				}

				else
				{
					score2 += 10;
				}

				pthread_mutex_unlock(&mutexScore);
				pthread_cond_signal(&condScore);
				pthread_kill(threadVaisseauAmiral, SIGCHLD);
				fini = 1;
				break;
		}

		pthread_mutex_unlock(&mutexGrille);
		sleepThread(80000000);
	}

	pthread_mutex_lock(&mutexGrille);

	if(tab[pMissile->ligne+1][pMissile->colonne] == MISSILE)
	{
		tab[pMissile->ligne+1][pMissile->colonne] = VIDE;
		DessineVide(pMissile->ligne+1, pMissile->colonne);
	}

	pthread_mutex_unlock(&mutexGrille); 
	pthread_cleanup_pop(1);
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadMissileFin(void *t)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	pthread_mutex_lock(&mutexListeMissiles); 

	if(retireMissile((S_MISSILE *) t))
	{
		free((S_MISSILE *) t);
	}

	pthread_mutex_unlock(&mutexListeMissiles);
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadTimeOut(void *t)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	sleepThread(600000000);
	((S_CANON *) t)->fireOn = 1;
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadInvaders(void *t)
{
	sigset_t mask;
	int temporisationFlotte =  1000000000;
	timespec_t temporisationFlotteSpec = sleepThread(temporisationFlotte);

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);

    while(1)
    {
	  	// lancement du thread flotte aliens
  	  	if((errno = pthread_create(&threadFlotteAliens, NULL, \
  	  		fctThreadFlotteAliens, &temporisationFlotteSpec)))
		{
			Trace("(THREAD_INVADERS) Err. de pthread_create");
			pthread_exit(NULL);
  	  	}

	  	pthread_join(threadFlotteAliens, NULL);
		pthread_mutex_lock(&mutexFlotteAliens);

		if(!nbAliens)
		{
			pthread_mutex_lock(&mutexScore);
			level++;
			pthread_mutex_unlock(&mutexScore);
			pthread_cond_signal(&condScore);
			temporisationFlotte -= temporisationFlotte / 100 * 30;
			temporisationFlotteSpec = sleepThread(temporisationFlotte);
		}

		else
		{
			pthread_kill(threadCanon1, SIGQUIT);
			pthread_kill(threadCanon2, SIGQUIT);
			temporisationFlotte =  1000000000;
			temporisationFlotteSpec = sleepThread(temporisationFlotte);
		}

		lh = 1, lb = 9, cg = 0, cd = 12, nbAliens = 35;
		pthread_mutex_unlock(&mutexFlotteAliens);
		pthread_mutex_lock(&mutexGrille);
		InitialiseBoucliers();
		pthread_mutex_unlock(&mutexGrille);
	}

	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadFlotteAliens(void *t)
{
	sigset_t mask;
	int aller = 1;
	int ok = 1;
	int alien = 0;
	timespec_t *temporisationFlotte = (timespec_t *) t;
	S_BOMBE *newBombe;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	pthread_mutex_lock(&mutexFlotteAliens);
	DessinerFlotte(lh, lb, cg, cd);
	pthread_mutex_unlock(&mutexFlotteAliens);

	while(ok)
	{
		pthread_mutex_lock(&mutexFlotteAliens);
		pthread_mutex_lock(&mutexGrille);
		Trace("AVANT DEPLACEMENT -> NBALIENS: %d cg: %d | cd:%d | lh:%d | lb:%d", \
			nbAliens, cg, cd, lh, lb);

		if((lb < NB_LIGNE-3) && (nbAliens > 0))
		{
			if(nbAliens > 0)
			{
				if((alien % 2) == 0)
				{
					newBombe = (S_BOMBE *) malloc(sizeof(S_BOMBE));
					pickUpAlien(&(newBombe->ligne), &(newBombe->colonne));

					// lancement d'un thread bombe
    				if((errno = pthread_create(&threadBombe, NULL, \
    					fctThreadBombe, newBombe)))
    				{
    					Trace("(THREAD_FLOTTE_ALIENS) Err. de pthread_create");
    					exit(1);
    				}

					Trace("\n\n\n ERRREURRR \n\n\n");
				}

				alien++;
			}

			if(aller)
			{
				if(nbAliens > 0)
				{
					if(cd+1 < NB_COLONNE)
					{ 
						if(DeplacerFlotte(lh, lh, lb, lb, cg, cg+1, cd, cd+1))
						{
							Verification_Toute_Ligne_Colonne_Tuee(lh, cg+1); 
							Verification_Toute_Ligne_Colonne_Tuee(lb, cd+1); 
						}

						cg++;
						cd++;
					}

					else
					{
						aller = 0;

						if(DeplacerFlotte(lh, lh, lb, lb, cg, cg-1, cd, cd-1))
						{
							Verification_Toute_Ligne_Colonne_Tuee(lh, cg-1); 
							Verification_Toute_Ligne_Colonne_Tuee(lb, cd-1); 
						}

						cg--;
						cd--;
							
					}
				}

				else
				{
					ok = 0;
				}
			}

			else    
			{
				if(nbAliens > 0)
				{
					if(cg-1 >= 0)
					{
						if(DeplacerFlotte(lh, lh, lb, lb, cg, cg-1, cd, cd-1))
						{
							Verification_Toute_Ligne_Colonne_Tuee(lh, cg-1); 
							Verification_Toute_Ligne_Colonne_Tuee(lb, cd-1); 
						}

						cd--;
						cg--;
						
					}

					else
					{
						aller = 1;

						if(DeplacerFlotte(lh, lh+1, lb, lb+1, cg, cg, cd, cd))
						{
							Verification_Toute_Ligne_Colonne_Tuee(lh+1, cg); 
							Verification_Toute_Ligne_Colonne_Tuee(lb+1, cd); 
						}

						lh++;
						lb++;	
					}
				}

				else
				{
					ok = 0;
				}
			}

			Trace("APRES DEPLACEMENT -> NBALIENS: %d cg: %d | cd:%d | lh:%d | lb:%d", \
				nbAliens, cg, cd, lh, lb);
		}

		else
		{
			ok = 0;
		}

		pthread_mutex_unlock(&mutexFlotteAliens);   
		pthread_mutex_unlock(&mutexGrille);
		nanosleep(temporisationFlotte, NULL);
	}

	pthread_mutex_lock(&mutexFlotteAliens);
	pthread_mutex_lock(&mutexGrille);
	SupprimerFlotte(lh, lb, cg, cd);
	pthread_mutex_unlock(&mutexFlotteAliens);
	pthread_mutex_unlock(&mutexGrille);
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadScore(void *t)
{
	sigset_t mask;
	char Level[5];
	char Vies1[5];
	char Vies2[5];
	char Score1[5];
	char Score2[5];

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	SauveCurseur();
	EffEcran();
	pthread_mutex_lock(&mutexScore);

	if(nbJoueurs == 1)
	{
		while(1)
		{
			sprintf(Level, "%d", level);
			pthread_mutex_lock(&mutexVies);
			sprintf(Vies1, "%d", nbVies1);
			pthread_mutex_unlock(&mutexVies);
			sprintf(Score1, "%d", score1);
			AffChaine("Niveau : ", 2, 0, 0);
			AffChaine(Level, 2, 10, 0);
			AffChaine("Joueur 1", 4, 0, 1);
			AffChaine("Vies :", 6, 0, 0);
			AffChaine(Vies1, 6, 8, 0);
			AffChaine("Score :", 8, 0, 0);
			AffChaine(Score1, 8, 9, 0);
			AffChaine("", 9, 0, 0);
			pthread_cond_wait(&condScore, &mutexScore);
		}
	}

	else
	{
		while(1)
		{
			sprintf(Level, "%d", level);
			pthread_mutex_lock(&mutexVies);
			sprintf(Vies1, "%d", nbVies1);
			sprintf(Vies2, "%d", nbVies2);
			pthread_mutex_unlock(&mutexVies);
			sprintf(Score1, "%d", score1);
			sprintf(Score2, "%d", score2);
			AffChaine("Niveau : ", 2, 0, 0);
			AffChaine(Level, 2, 10, 0);
			AffChaine("Joueur 1", 4, 0, 1);
			AffChaine("Joueur 2", 4, 20, 1);
			AffChaine("Vies :", 6, 0, 0);
			AffChaine(Vies1, 6, 8, 0);
			AffChaine("Vies :", 6, 20, 0);
			AffChaine(Vies2, 6, 27, 0);
			AffChaine("Score :", 8, 0, 0);
			AffChaine(Score1, 8, 9, 0);
			AffChaine("Score :", 8, 20, 0);
			AffChaine(Score2, 8, 28, 0);
			AffChaine("", 9, 0, 0);
			pthread_cond_wait(&condScore, &mutexScore);
		}
	}

	pthread_mutex_unlock(&mutexScore);
	RestitueCurseur();
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadBombe(void *t)
{
	sigset_t mask;
	S_BOMBE *pBombe = (S_BOMBE *) t;
	int finiJournee = 0;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	sigemptyset(&Action.sa_mask);
	Action.sa_flags = 0;
	Action.sa_handler = HandlerSigInt;

	if(sigaction(SIGINT, &Action, NULL) == -1) 
   	{
		Trace("(THREAD_BOMBE) Err. de sigaction");
		exit(1);
   	}

	pBombe->tid = pthread_self();
	pBombe->suivant = NULL;
	pthread_cleanup_push(fctThreadBombeFin, pBombe);
	pthread_mutex_lock(&mutexListeBombes);
	insereBombe(pBombe);
	pthread_mutex_unlock(&mutexListeBombes);

	while((!finiJournee) && (pBombe->ligne < NB_LIGNE))
	{
		Trace("(THREAD_BOMBE) %d Ligne %d Colonne %d\n", \
			pBombe->tid, pBombe->ligne, pBombe->colonne);
		pthread_mutex_lock(&mutexGrille);

		switch(tab[pBombe->ligne][pBombe->colonne])
		{
			case VIDE:
				if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
				{
					tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
					DessineVide(pBombe->ligne-1, pBombe->colonne);
				}

				tab[pBombe->ligne][pBombe->colonne] = BOMBE;
				DessineBombe(pBombe->ligne, pBombe->colonne);
				pBombe->ligne++;
				break;
			case MISSILE:
				if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
				{
					tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
					DessineVide(pBombe->ligne-1, pBombe->colonne);
				}

				tab[pBombe->ligne][pBombe->colonne] = VIDE;
				DessineVide(pBombe->ligne, pBombe->colonne);
				pthread_mutex_lock(&mutexListeMissiles);
				pthread_kill(getTidMissile(pBombe->ligne, pBombe->colonne), SIGINT);
				pthread_mutex_unlock(&mutexListeMissiles);
				finiJournee = 1;
				break;
			case BOUCLIER1:
				if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
				{
					tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
					DessineVide(pBombe->ligne-1, pBombe->colonne);
				}

				tab[pBombe->ligne][pBombe->colonne] = BOUCLIER2;
				DessineBouclier(pBombe->ligne, pBombe->colonne, 2);
				finiJournee = 1;
				break;
			case BOUCLIER2:
				if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
				{
					tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
					DessineVide(pBombe->ligne-1, pBombe->colonne);
				}

				tab[pBombe->ligne][pBombe->colonne] = VIDE;
				DessineVide(pBombe->ligne, pBombe->colonne);
				finiJournee = 1;
				break;
			case CANON1:
				if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
				{
					tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
					DessineVide(pBombe->ligne-1, pBombe->colonne);
				}

				pthread_kill(threadCanon1, SIGQUIT);
				finiJournee = 1;
				break;
			case CANON2:
				if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
				{
					tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
					DessineVide(pBombe->ligne-1, pBombe->colonne);
				}

				pthread_kill(threadCanon2, SIGQUIT);
				finiJournee = 1;
				break;
			case ALIEN:
				if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
				{
					tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
					DessineVide(pBombe->ligne-1, pBombe->colonne);
				}

				finiJournee = 1;
				break;
		}

		pthread_mutex_unlock(&mutexGrille);
		sleepThread(160000000);
	}

	pthread_mutex_lock(&mutexGrille);

	if(tab[pBombe->ligne-1][pBombe->colonne] == BOMBE)
	{
		tab[pBombe->ligne-1][pBombe->colonne] = VIDE;
		DessineVide(pBombe->ligne-1, pBombe->colonne);
	}

	pthread_mutex_unlock(&mutexGrille);
	pthread_cleanup_pop(1);
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadBombeFin(void *t)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	pthread_mutex_lock(&mutexListeBombes); 

	if(retireBombe((S_BOMBE *) t))
	{
		free((S_BOMBE *) t);
	}

	pthread_mutex_unlock(&mutexListeBombes);
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void *fctThreadVaisseauAmiral(void *t)
{
	int randomCol = 0, randomDir = 0, randomAlarm = 0;

	sigemptyset(&Action.sa_mask);
	Action.sa_flags = 0;
	Action.sa_handler = HandlerSigChld;

	if(sigaction(SIGCHLD, &Action, NULL) == -1) 
   	{
		Trace("(THREAD_CANON) Err. de sigaction");
     	exit(1);
   	}

	while(1)
	{
		terminerVaisseauAmiral = 0;
		pthread_mutex_lock(&mutexFlotteAliens);

		while((nbAliens % 6) != 0)
		{
			pthread_cond_wait(&condFlotteAliens, &mutexFlotteAliens);
		}

		pthread_mutex_unlock(&mutexFlotteAliens);
		pthread_mutex_lock(&mutexGrille);

		do
		{
			randomCol = rand() % (NB_COLONNE-1-1) + 1;
		} while(tab[0][randomCol] != VIDE);

		tab[0][randomCol] = VAISSEAU_AMIRAL;
		DessineVaisseauAmiral(0, randomCol);
		pthread_mutex_unlock(&mutexGrille);
		randomDir = rand() % (1-0) + 0;
		randomAlarm = rand() % (12-4) + 4;
		alarm(randomAlarm);

		if(!randomDir) // gauche
		{
			while((randomCol > 0) && (!terminerVaisseauAmiral))
			{
				pthread_mutex_lock(&mutexGrille);
				tab[0][randomCol] = VIDE;
				DessineVide(0, randomCol);
				randomCol--;

				if(tab[0][randomCol] == MISSILE)
				{
					tab[0][randomCol] = VIDE;
					DessineVide(0, randomCol);
					pthread_mutex_lock(&mutexListeMissiles);
					pthread_mutex_lock(&mutexScore);

					if(getMissile(0, randomCol)->joueur == 1)
					{
						score1 += 10;
					}

					else
					{
						score2 += 10;
					}

					pthread_mutex_unlock(&mutexListeMissiles);
					pthread_mutex_unlock(&mutexScore);
					pthread_cond_signal(&condScore);
					pthread_cond_signal(&condFlotteAliens);
					pthread_mutex_unlock(&mutexGrille);
					alarm(0);
					pthread_mutex_lock(&mutexListeMissiles);
					pthread_kill(getTidMissile(0, randomCol), SIGINT);
					pthread_mutex_unlock(&mutexListeMissiles);
					break;
				}

				else
				{
					tab[0][randomCol] = VAISSEAU_AMIRAL;
					DessineVaisseauAmiral(0, randomCol);
					pthread_mutex_unlock(&mutexGrille);
					sleepThread(800000000);
				}
			}
		}

		else // droite
		{
			while((randomCol < NB_COLONNE-1) && (!terminerVaisseauAmiral))
			{
				pthread_mutex_lock(&mutexGrille);
				tab[0][randomCol] = VIDE;
				DessineVide(0, randomCol);
				randomCol++;

				if(tab[0][randomCol] == MISSILE)
				{
					tab[0][randomCol] = VIDE;
					DessineVide(0, randomCol);
					pthread_mutex_lock(&mutexListeMissiles);
					pthread_mutex_lock(&mutexScore);

					if(getMissile(0, randomCol)->joueur == 1)
					{
						score1 += 10;
					}

					else
					{
						score2 += 10;
					}

					pthread_mutex_unlock(&mutexListeMissiles);
					pthread_mutex_unlock(&mutexScore);
					pthread_cond_signal(&condScore);
					pthread_cond_signal(&condFlotteAliens);
					pthread_mutex_unlock(&mutexGrille);
					alarm(0);
					pthread_mutex_lock(&mutexListeMissiles);
					pthread_kill(getTidMissile(0, randomCol), SIGINT);
					pthread_mutex_unlock(&mutexListeMissiles);
					break;
				}

				else
				{
					tab[0][randomCol] = VAISSEAU_AMIRAL;
					DessineVaisseauAmiral(0, randomCol);
					pthread_mutex_unlock(&mutexGrille);
					sleepThread(200000000);
				}
			}
		}

		pthread_mutex_lock(&mutexGrille);
		tab[0][randomCol] = VIDE;
		DessineVide(0, randomCol);
		pthread_mutex_unlock(&mutexGrille);
	}

	pthread_exit(NULL);
}		

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void HandlerSigUsr1(int Sig)
{
	S_CANON *structCanon = (S_CANON *) pthread_getspecific(cleCanon);

	Trace("Reception d'un SIGUSR1 venant du thread %d", pthread_self());
	pthread_mutex_lock(&mutexGrille);

	if((structCanon->colonne > 0) && (tab[NB_LIGNE-1][structCanon->colonne-1] == VIDE))
	{
		tab[NB_LIGNE-1][structCanon->colonne] = VIDE;
		DessineVide(NB_LIGNE-1, structCanon->colonne);
		structCanon->colonne--;
		tab[NB_LIGNE-1][structCanon->colonne] = structCanon->joueur;
		DessineCanon(NB_LIGNE-1, structCanon->colonne, structCanon->joueur);
		Trace("(THREAD_CANON) Ligne %d Colonne %d", NB_LIGNE-1, structCanon->colonne);
	}

	pthread_mutex_unlock(&mutexGrille);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void HandlerSigUsr2(int Sig)
{
	S_CANON *structCanon = (S_CANON *) pthread_getspecific(cleCanon);

	Trace("Reception d'un SIGUSR2 thread %d", pthread_self());
	pthread_mutex_lock(&mutexGrille);

	if((structCanon->colonne < NB_COLONNE-1) && \
		(tab[NB_LIGNE-1][structCanon->colonne+1] == VIDE))
	{
		tab[NB_LIGNE-1][structCanon->colonne] = VIDE;
		DessineVide(NB_LIGNE-1, structCanon->colonne);
		structCanon->colonne++;
		tab[NB_LIGNE-1][structCanon->colonne] = structCanon->joueur;
		DessineCanon(NB_LIGNE-1, structCanon->colonne, structCanon->joueur);
		Trace("(THREAD_CANON) Ligne %d Colonne %d", NB_LIGNE-1, structCanon->colonne);
	}

	pthread_mutex_unlock(&mutexGrille);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void HandlerSigHup(int Sig)
{
	S_MISSILE *newMissile;
	S_CANON *structCanon = (S_CANON *) pthread_getspecific(cleCanon);

	Trace("Reception d'un SIGHUP venant du thread %d", pthread_self());

	if(structCanon->fireOn)
	{
		structCanon->fireOn = 0;
		newMissile = (S_MISSILE *) malloc(sizeof(S_MISSILE));
		newMissile->colonne = structCanon->colonne;
		newMissile->ligne = NB_LIGNE - 2;
		newMissile->joueur = structCanon->joueur;
		newMissile->suivant = NULL;

		// lancement d'un thread missile
    	if((errno = pthread_create(&threadMissile, NULL, \
    		fctThreadMissile, newMissile)))
    	{
    		Trace("(THREAD_CANON) Err. de pthread_create");
    		exit(1);
    	}

		pthread_detach(threadMissile);

    	// lancement du thread timeout
    	if((errno = pthread_create(&threadTimeOut, NULL, fctThreadTimeOut, structCanon)))
    	{
    		Trace("(THREAD_CANON) Err. de pthread_create");
    		exit(1);
    	}    	
    }
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void HandlerSigInt(int Sig)
{
	Trace("Reception d'un SIGINT thread %d", pthread_self());
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void HandlerSigQuit(int Sig)
{
	S_CANON *structCanon = (S_CANON *) pthread_getspecific(cleCanon);

	Trace("Reception d'un SIGQUIT thread %d", pthread_self());
	pthread_mutex_lock(&mutexGrille);
	tab[NB_LIGNE-1][structCanon->colonne] = VIDE;
	DessineVide(NB_LIGNE-1, structCanon->colonne);
	pthread_mutex_unlock(&mutexGrille);
	pthread_mutex_lock(&mutexVies);

	if(structCanon->joueur == 1)
	{
		nbVies1--;
		enVie1 = 0;
	}

	else
	{
		nbVies2--;
		enVie2 = 0;
	}

	pthread_mutex_unlock(&mutexVies);
	pthread_cond_signal(&condVies);
	pthread_cond_signal(&condScore);
	pthread_exit(NULL);
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void HandlerSigChld(int Sig)
{
	Trace("Reception d'un SIGCHLD thread %d", pthread_self());
	alarm(0);
	terminerVaisseauAmiral = 1;
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void insereMissile(S_MISSILE *pm)
{
	S_MISSILE *pTemp;

	if(pListeMissiles == NULL)
	{
		pListeMissiles = pm;
	}

	else
	{
		pTemp = pListeMissiles;

		while(pTemp->suivant != NULL)
		{
			pTemp = pTemp->suivant;
		}

		pTemp->suivant = pm;
	}
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

pthread_t getTidMissile(int l, int c)
{
	S_MISSILE *tmp;

	if((tmp = getMissile(l, c)))
	{
		return tmp->tid;
	}
}	

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

S_MISSILE *getMissile(int l, int c)
{
	S_MISSILE *pTemp;

	pTemp = pListeMissiles;

	while(pTemp != NULL)
	{
		if((pTemp->ligne == l) && (pTemp->colonne == c))
		{
			return pTemp;
		}

		pTemp = pTemp->suivant;
	} 

	return pListeMissiles;
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

S_MISSILE *retireMissile(S_MISSILE *pm)
{
	S_MISSILE *pTemp;
	S_MISSILE *pPrec = NULL;

	pTemp = pListeMissiles;

	if((pListeMissiles->ligne == pm->ligne) && \
		(pListeMissiles->colonne == pm->colonne))
	{
		pListeMissiles = pListeMissiles->suivant;
		return pTemp;
	}

	pTemp = pListeMissiles;

	while(pTemp->suivant != pm)
	{
		pTemp = pTemp->suivant;
	}

	pTemp->suivant = pm->suivant;
	pm->suivant = NULL;
	return pm; 
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

timespec_t sleepThread(int ns)
{
	timespec_t temps;

	temps.tv_nsec = ns % 1000000000;
	temps.tv_sec = ns / 1000000000;
	nanosleep(&temps, NULL);
	return temps;
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void Verification_Toute_Ligne_Colonne_Tuee(int l, int c)
{
	int PLUS_PERSONNE_SUR_LA_LIGNE = 1;
	int PLUS_PERSONNE_SUR_LA_COLONNE = 1;

	for(int i = cg; i < cd; i += 2)
	{
		if(tab[l][i] == ALIEN)
		{
			PLUS_PERSONNE_SUR_LA_LIGNE = 0;
		}
	}

	if(PLUS_PERSONNE_SUR_LA_LIGNE)
	{
		if(l == lb)
		{
			lb -= 2;
		}

		if(l == lh)
		{
			lh += 2;
		}
	}

	for(int i = lh; i < lb; i += 2)
	{
		if(tab[i][c] == ALIEN)
		{
			PLUS_PERSONNE_SUR_LA_COLONNE = 0;
		}
	}

	if(PLUS_PERSONNE_SUR_LA_COLONNE)
	{
		if(c == cd)
		{
			cd = cd-2;
		}

		if(c == cg)
		{
			cg += 2;
		}
	}
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

int DeplacerFlotte(int l, int newL, int maxL, int newMaxL, \
	int c, int newC, int maxC, int newMaxC)
{
	int tmp = c;
	int newTmp = newC;
	int tuer = 0;
	S_BOMBE *newBombe;

	while(newL <= newMaxL)
	{
		c = tmp;
		newC = newTmp;

		while(newC <= newMaxC)
		{	
			if(tab[l][c] == ALIEN)
			{
				if(tab[newL][newC] == MISSILE)
				{
					DessineVide(newL, newC);
					tab[newL][newC] = VIDE;
					tuer = 1;
				}

				else
				{
					if(tab[newL][newC] == BOMBE)
					{
						DessineAlien(newL, newC);
						tab[newL][newC] = ALIEN;
						pthread_mutex_lock(&mutexListeBombes);
						pthread_kill(getTidBombe(newL, newC), SIGINT);
						pthread_mutex_unlock(&mutexListeBombes);
						newBombe = (S_BOMBE *) malloc(sizeof(S_BOMBE));
						newBombe->ligne = newL + 1;
						newBombe->colonne = newC;

						// lancement d'un thread bombe
    					if((errno = pthread_create(&threadBombe, NULL, \
    						fctThreadBombe, newBombe)))
    					{
    						Trace("(THREAD_FLOTTE_ALIENS) Err. de pthread_create");
    						exit(1);
    					}
					}

					else
					{
						DessineAlien(newL, newC);
						tab[newL][newC] = ALIEN;
					}
				}

				tab[l][c] = VIDE;
				DessineVide(l,c);
			}

			if(tuer)
			{
			    tuer = 0;
			    nbAliens--;
				pthread_mutex_lock(&mutexListeMissiles);
				pthread_mutex_lock(&mutexScore);

				if(getMissile(newL, newC)->joueur == 1)
				{
					score1++;
				}

				else
				{
					score2++;
				}

				pthread_mutex_unlock(&mutexListeMissiles);
				pthread_mutex_unlock(&mutexScore);
				pthread_cond_signal(&condScore);
				pthread_cond_signal(&condFlotteAliens);
			    pthread_mutex_lock(&mutexListeMissiles);
				pthread_kill(getTidMissile(newL,newC), SIGINT);
			    pthread_mutex_unlock(&mutexListeMissiles); 	
			}

			c += 2;
			newC += 2;
		}

		l += 2;
		newL += 2;
	}

	return tuer;
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void SupprimerFlotte(int l, int maxL, int c, int maxC)
{
	int tmp = c;

	while(l <= maxL)
	{
		c = tmp;

		while(c <= maxC)
		{
			if(tab[l][c] == ALIEN)
			{
				tab[l][c] = VIDE;
				DessineVide(l, c);
			}

			c += 2;
		}

		l += 2;
	}
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void DessinerFlotte(int l,int maxL, int c, int maxC)
{
	int tmp = c;

	while(l <= maxL)
	{
		c = tmp;

		while(c <= maxC)
		{
			pthread_mutex_lock(&mutexGrille);
			tab[l][c] = ALIEN;
			DessineAlien(l, c);
			pthread_mutex_unlock(&mutexGrille);
			c += 2;
		}

		l += 2;
	}
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void insereBombe(S_BOMBE *pb)
{
	S_BOMBE *pTemp;

	if(pListeBombes == NULL)
	{
		pListeBombes = pb;
	}

	else
	{
		pTemp = pListeBombes;

		while(pTemp->suivant != NULL)
		{
			pTemp = pTemp->suivant;
		}

		pTemp->suivant = pb;
	}
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

pthread_t getTidBombe(int l, int c)
{
	S_BOMBE *tmp;

	if((tmp = getBombe(l, c)))
	{
		return tmp->tid;
	}
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

S_BOMBE *getBombe(int l, int c)
{
	S_BOMBE *pTemp;

	pTemp = pListeBombes;

	while(pTemp != NULL)
	{
		if((pTemp->ligne == l) && (pTemp->colonne == c))
		{
			return pTemp;
		}

		pTemp = pTemp->suivant;
	} 

	return pListeBombes;
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

S_BOMBE *retireBombe(S_BOMBE *pb)
{
	S_BOMBE *pTemp;
	S_BOMBE *pPrec = NULL;

	pTemp = pListeBombes;

	if((pListeBombes->ligne == pb->ligne) && \
		(pListeBombes->colonne == pb->colonne))
	{
		pListeBombes = pListeBombes->suivant;
		return pTemp;
	}

	pTemp = pListeBombes;

	while(pTemp->suivant != pb)
	{
		pTemp = pTemp->suivant;
	}

	pTemp->suivant = pb->suivant;
	pb->suivant = NULL;
	return pb; 
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void pickUpAlien(int *l, int *c)
{
	int pickUp = 0;
	int randomLine;
	int randomCol;
	int i;

	while(!pickUp)
	{
		if((lb-lh) && (cd-cg))
		{
			randomLine = rand() % (lb-lh) + lh;
			randomCol = rand() % (cd-cg) + cg;
		}

		if(!(lb-lh) && (cd-cg))
		{
			randomLine = lb;
			randomCol = rand() % (cd-cg) + cg;
		}

		if((lb-lh) && !(cd-cg))
		{
			randomLine = rand() % (lb-lh) + lh;
			randomCol = cd;
		}

		if(!(lb-lh) && !(cd-cg))
		{
			randomLine = lb;
			randomCol = cd;
		}

		if(tab[randomLine][randomCol] == ALIEN)
		{
			Trace("RandomLine %d RandomCol %d", randomLine, randomCol);
			*l = randomLine + 1;
			*c = randomCol;
			pickUp = 1;
		}
	}
}

/*¤º`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø*/

void freeSpecificZone(void *p)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	Trace("Liberation de la zone specifique");
	free(p);
}
