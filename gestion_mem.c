#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "se_fichier.c"

void * thread_fils(void * arg);
int lecture_tube(char* nom_tube);
void ecriture_tube(char* nom_tube, int num_send);
int LRU();

/*******************CONTIENT LES INFOS DU FICHIER .CFG****************/
typedef struct {
	int nombre_frames;
	int taille_page;
	int nombre_page;
	int nombre_threads;
	int nombre_daccess;
}MEMOIRE_CONFIG;

/******************DEFINISSIONS DES TYPES MEMOIRES************************/
typedef struct{
	int *memoire_rapide;
	int *memoire_lente;
}TYPE_MEMOIRE;

/*****************STRUCTURE DES MUTEX***********************/
typedef struct {
	pthread_mutex_t *mutex;
	pthread_cond_t *cond;
	int * compt;
} barriere_t;

/****************INTITIALISATION DE MEMOIRE_CONFIG*********************/
MEMOIRE_CONFIG _init_cfg(char* nomFic){

	MEMOIRE_CONFIG m_config;
	int erreur = 0;

	printf("Lecture du fichier de configuration....\n");

	SE_FICHIER f = SE_ouverture(nomFic, O_RDONLY);
	if(f.descripteur == -1){
		erreur = 1;
	}
	if(SE_lectureEntier(f, &m_config.nombre_frames) == -1){
		erreur = 2;;
	}
	if(SE_lectureEntier(f, &m_config.taille_page) == -1){
		erreur = 3;;
	}
	if(SE_lectureEntier(f, &m_config.nombre_page) == -1){
		erreur = 4;
	}
	if(SE_lectureEntier(f, &m_config.nombre_threads) == -1){
		erreur = 5;
	}
	if(SE_lectureEntier(f, &m_config.nombre_daccess) == -1){
		erreur = 6;
	}
	if(SE_fermeture(f) == -1){
		erreur = 7;
	}

	if(erreur != 0){
		printf("Erreur lors de la lecture du fichier de configuration. (Erreur n°%d)\n", erreur);
		exit(0);
	}
	printf("Fichier de configuration lu avec succès.\n");

	return m_config;
}

/***************INITIALISATION TYPE_MEMOIRE***********************/
TYPE_MEMOIRE _init_type_(MEMOIRE_CONFIG m_config){
	TYPE_MEMOIRE type;
	type.memoire_lente = malloc(sizeof(long int) * m_config.nombre_page);
	type.memoire_rapide = malloc(sizeof(int) * m_config.nombre_frames);

	for (int i = 0; i < m_config.nombre_frames; ++i)
	{
		type.memoire_rapide[i] = i + 1;
	}

	for (long int i = 0; i < m_config.nombre_page; ++i)
	{
		type.memoire_lente[i] = i;
	}

	return type;
}


/*************AFFICHAGE DE LA STRUCT MEMOIRE_CONFIG********************/
void print_memConfig(MEMOIRE_CONFIG m_config){
	printf("  >> Nombres de frames : %d\n", m_config.nombre_frames);
	printf("  >> Taille d'une page : %d\n", m_config.taille_page);
	printf("  >> Nombres de pages : %d\n", m_config.nombre_page);
	printf("  >> Nombres de threads fils : %d\n", m_config.nombre_threads);
	printf("  >> Nombres d'accès demandés par threads : %d\n", m_config.nombre_daccess);
}

/**************AFFICHAGE DE LA STRUCT TYPE_MEMOIRE*****************/
void print_type(TYPE_MEMOIRE type, MEMOIRE_CONFIG m_config){

	printf("(TEST)memoire_rapide : \n");
	for(int i = 0; i < m_config.nombre_frames; ++i){
		printf("  >> memoire_rapide[%d] : %d\n", i, type.memoire_rapide[i]);
	}
	printf("\n");
	printf("(TEST)memoire_lente \n:");
	for (int i = 0; i < m_config.nombre_page; ++i)
	{
		printf("  >> memoire_lente[%d] : %2d\n ", i, type.memoire_lente[i]);
	}
	printf("\n");
}

/**************OBTIENT LES ADRESSES*****************/
int* get_adr(MEMOIRE_CONFIG m_config){

	int* adresse = malloc(sizeof(int) * 10);
	//adresse[i] = adresse logiques et i = adresse physiques
	for (int i = 0; i < 10; ++i)
	{
		adresse[i] = m_config.nombre_frames * m_config.taille_page + (i % m_config.nombre_frames);
	}
	
	return adresse;
}

/*******************AFFICHAGE DES ADRESSES***************************/
void print_adresse(int* adresse){

	for (int i = 0; i < 10; ++i){
		printf("Adresse physique : %d / ", i);
		printf("Adresse logique : %d\n", adresse[i]);
	}
}

/****************FONCTION SUR LES THREADS****************/
//PERE
void create_threads(MEMOIRE_CONFIG m_config){
	pthread_t tid[m_config.nombre_threads];

	mkfifo("/tmp/tube1", 0666);
	mkfifo("/tmp/tube2", 0666);

	int num_receive = 0;
	
	printf("(TEST)Sortie thread fils :\n");
	for(int i = 0; i < m_config.nombre_threads; ++i){
		pthread_create(tid + i , NULL, thread_fils, NULL );

		num_receive = lecture_tube("/tmp/tube1");
		printf("num_receive = %d\n", num_receive);

		ecriture_tube("/tmp/tube2", num_receive + i);
	}
	for(int i = 0; i < m_config.nombre_threads; ++i){
		pthread_join(tid[i], NULL);
	}

	printf("\n");
	
	unlink("/tmp/tube1");
	unlink("/tmp/tube2");

	//ouvre tube 1 en tant que thread fils, ecrit dans tube1, ferme tube1
	//ouvre tube 1 en tant que thread pere, lit dans tube1, ferme tube1
	//ouvre tube2  en tant que thread pere, ecrit dans tube2, ferme tube2
	//ouvre tube2 en tant que thread fils, lit dans tube2, ferme tube2

}

//FILS
void * thread_fils(void * arg){
	int num_send = 5;
	int num_receive = 0;
	printf("  > creation du tid du thread : %ld\n", pthread_self());
	for(int i = 0; i < 100; ++i);
	ecriture_tube("/tmp/tube1", num_send);

	num_receive = lecture_tube("/tmp/tube2");
	printf("num_receive = %d\n", num_receive);
	printf("  < suppression du tid du thread : %ld\n", pthread_self());


	return NULL;
}
		
/****************FONCTION SUR LES TUBES****************/
int lecture_tube(char* nom_tube){
	int num_receive = 0;
	int fd;
	if((fd = open(nom_tube, O_RDONLY)) == -1){
		perror("open_pere() ");
		exit(0);
	}
	if(read(fd, &num_receive, sizeof(num_receive)) == -1){
		perror("open_fils() ");
		exit(0);
	}
	close(fd);

	return num_receive;

}

void ecriture_tube(char* nom_tube, int num_send){
	int fd;
	if((fd = open(nom_tube, O_WRONLY)) == -1){
		perror("open() ");
		exit(0);
	}
	
	if(write(fd, &num_send, sizeof(int)) == -1){
		printf("write() : %s\n", strerror(errno));
		exit(0);
	}
	close(fd);

}

/**********************LRU********************/

int LRU(){
	int lastArrived = 0;


	return lastArrived;
}


/******************LIBERATION MEMOIRE*****************/
void free_type(TYPE_MEMOIRE type){
	free(type.memoire_lente);
	free(type.memoire_rapide);
}

/************PROGRAMME PRINCIPALE***************/
int main(void){
	int* adresse = malloc(sizeof(int));

	MEMOIRE_CONFIG m_config= _init_cfg("config.cfg");
	TYPE_MEMOIRE type = _init_type_(m_config);
	print_memConfig(m_config);
	adresse = get_adr(m_config);
	create_threads(m_config);
	free_type(type);
	free(adresse);
	
	return 0;
}