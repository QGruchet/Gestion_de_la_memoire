#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include "se_fichier.c"

void * thread_fils(void * arg);
void create_tube(char* path);
void write_tube(void* message, int fd);
void read_tube(void* message, int fd);

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


/*************AFFICHAGE DU FICHIER********************/
void affiche_memConfig(MEMOIRE_CONFIG m_config){
	printf("  >> Nombres de frames : %d\n", m_config.nombre_frames);
	printf("  >> Taille d'une page : %d\n", m_config.taille_page);
	printf("  >> Nombres de pages : %d\n", m_config.nombre_page);
	printf("  >> Nombres de threads fils : %d\n", m_config.nombre_threads);
	printf("  >> Nombres d'accès demandés par threads : %d\n", m_config.nombre_daccess);
}

/**************AFFICHAGE DE LA STRUCT TYPE_MEMOIRE*****************/
void affiche_type(TYPE_MEMOIRE type, MEMOIRE_CONFIG m_config){

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
void affiche_adresse(int* adresse){

	for (int i = 0; i < 10; ++i){
		printf("Adresse physique : %d / ", i);
		printf("Adresse logique : %d\n", adresse[i]);
	}
}

/****************CREATION DES THREADS****************/
//PERE
void create_threads(MEMOIRE_CONFIG m_config){
	pthread_t tid[m_config.nombre_threads];
	barriere_t bar[m_config.nombre_threads];
	pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;
	int compt;
	int fd;
	char * nameTube= "/tmp/tube";

	create_tube(nameTube);
	fd = open(nameTube, O_RDWR);
	close(fd);
	unlink(nameTube);

	//CREATION THREADS
	compt = m_config.nombre_threads;
	printf("(TEST)Sortie thread fils :\n");
	for (int i = 0; i < m_config.nombre_threads; ++i)
	{
		bar[i].compt = &compt;
		bar[i].mutex = &mut;
		bar[i].cond = &cnd;
		pthread_create(tid + i, NULL, thread_fils, bar + i);
	}

	for (int i = 0; i < m_config.nombre_threads; ++i){
		pthread_join(tid[i], NULL);
	}
	printf("\n");

}

//FILS
void * thread_fils(void * arg){
	barriere_t *bar = (barriere_t *)arg;

	pthread_mutex_lock(bar->mutex);
	printf("  >> Entrer barriere\n");
	(*bar->compt)--;
	if(*bar->compt != 0){
		pthread_cond_wait(bar->cond, bar->mutex);
	}
	else{
		pthread_cond_broadcast(bar->cond);
	}
	printf("  << Sortie barriere\n");
	pthread_mutex_unlock(bar->mutex);

	return NULL;
}
		
/****************FONCTIONS SUR LES TUBES****************/
//CHECK SI CHEMIN EXISTE
int check_tube(char* path){

	struct stat s;
    if(stat(path, &s) == 0) return 1;
    return 0;
}

//CREATION
void create_tube(char* path){

	if(!check_tube(path)){
		if(mkfifo(path, 0666) < 0){
			perror("mkfifo() ");
			exit(-1);
		}
	}
}

//LECTURE
void read_tube(void* message, int fd){

	if(read(fd, message, strlen(message)) < 0){
		perror("read() ");
		exit(-1);
	}
}

//ECRITURE
void write_tube(void* message, int fd){

	if(write(fd, message, strlen(message)) < 0){
		perror("write() ");
		exit(-1);
	}
}


/******************LIBERATION MEMOIRE*****************/
void free_type(TYPE_MEMOIRE type){
	free(type.memoire_lente);
	free(type.memoire_rapide);
}

/************PROGRAMME PRINCIPALE***************/
int main(void){
	int* adresse = malloc(sizeof(int) * 10);

	MEMOIRE_CONFIG m_config= _init_cfg("config.cfg");
	TYPE_MEMOIRE type = _init_type_(m_config);
	affiche_memConfig(m_config);
	adresse = get_adr(m_config);
	//affiche_adresse(adresse);
	create_threads(m_config);
	//affiche_type(type, m_config);
	free_type(type);
	free(adresse);
	
	return 0;
}
