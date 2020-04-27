#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "se_fichier.c"

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

/****************LECTURE DU FICHIER*********************/
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

/***************INITIALISATION TYPE DE MEMOIRE***********************/
TYPE_MEMOIRE _init_type_(){
	TYPE_MEMOIRE type;
	type.memoire_lente = malloc(sizeof(int) * 10);
	type.memoire_rapide = malloc(sizeof(int) * 10);

	for (int i = 0; i < 10; ++i)
	{
		type.memoire_rapide[i] = i + 10;
		type.memoire_lente[i] = i;
	}

	return type;
}


/*************AFFICHAGE DU FICHIER********************/
void affiche_struct(MEMOIRE_CONFIG m_config){
	printf("  >> Nombres de frames : %d\n", m_config.nombre_frames);
	printf("  >> Taille d'une page : %d\n", m_config.taille_page);
	printf("  >> Nombres de pages : %d\n", m_config.nombre_page);
	printf("  >> Nombres de threads fils : %d\n", m_config.nombre_threads);
	printf("  >> Nombres d'accès demandés par threads : %d\n", m_config.nombre_daccess);
}

/**************AFFICHAGE DE LA STRUCT TYPE_MEMOIRE*****************/
void affiche_type(TYPE_MEMOIRE type){

	printf("memoire_rapide :");
	for (int i = 0; i < 10; ++i)
	{
		printf("%3d", type.memoire_rapide[i]);
	}
	printf("\n");
	printf("memoire_lente :");
	for (int i = 0; i < 10; ++i)
	{
		printf("%2d", type.memoire_lente[i]);
	}
	printf("\n");
}

/**************OBTIENT L'ADRESSE PHYSIQUE*****************/
int get_adr_phy(MEMOIRE_CONFIG m_config){

	int adr_phy = 0;
	adr_phy = m_config.taille_page * m_config.nombre_page ;

	return adr_phy;
}


/************PROGRAMME PRINCIPALE***************/
int main(void){
	int adr_phy = 0;

	MEMOIRE_CONFIG m_config= _init_cfg("config.cfg");
	TYPE_MEMOIRE type = _init_type_();
	affiche_struct(m_config);
	adr_phy = get_adr_phy(m_config);
	printf("adr_phy : 0 -> %d\n", adr_phy);
	affiche_type(type);
	
	return 0;
}