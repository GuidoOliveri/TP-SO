/*
 * libreriaMapa.c
 *
 *  Created on: 7/9/2016
 *      Author: utnso
 */

#include "libreriaMapa.h"
#include "libSockets.h"
#include <tad_items.h>

#define MAX_LEN 128

pthread_mutex_t mutexPaqueton = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mupaq = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t muSem = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexMortenAtend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexMurte = PTHREAD_MUTEX_INITIALIZER;

int numEntrenador;

t_infoCliente clientesActivos[2048];

extern t_list* entrenadoresEnCurso;
extern t_list* pokemons;
extern sem_t sem_Listos;
extern sem_t sem_quantum;
extern sem_t sem_llego;
extern char* nombreMapa;
extern metaDataComun* datosMapa;
extern metaDataComun* datos2;
extern t_log* logs;
extern t_queue* colaListos;
extern t_list* entrenadoresEnCurso;
extern t_list* pokenests;
extern t_list* items;
extern t_list* disponibles;
extern t_list* listaContenedora;
extern t_list* colaDeListosImp;
extern t_list* colaDeBloqImp;

/////////////////////////////////////////////////////////////////////////////
void* recibirDatos(int conexion, int tamanio) {
	void* mensaje = (void*) malloc(tamanio);
	int bytesRecibidos = recv(conexion, mensaje, tamanio, MSG_WAITALL);
	if (bytesRecibidos != tamanio) {
		perror("Error al recibir el mensaje\n");
		free(mensaje);
		char* adios = string_new();
		string_append(&adios, "0\0");
		return adios;
	}
	return mensaje;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void imprimirGiladas(void *unCliente) {

	t_infoCliente *infoCliente = (t_infoCliente *) unCliente;
	char paquete[1024];
	int status = 1;

	void enviarAvisoDeCierre() {
		printf("\n");
		enviarHeader(infoCliente->socket, 9);
		printf(
				"AVISO: cierre inesperando del sistema. Notificando a los clientes y finalizando...\n");
		exit(0);
	}

	printf("Entrenador #%d conectado! esperando mensajes... \n",
			infoCliente->cliente);

	signal(SIGINT, enviarAvisoDeCierre);

	while (status != 0) {
		status = recv(infoCliente->socket, (void*) paquete, 1024, 0);
		if (status != 0) {
			printf("el Entrenador #%d dijo: \n %s", infoCliente->cliente,
					paquete);
			enviarHeader(infoCliente->socket, 1);
		}

	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
int calcularDistancia(entrenador* ent) {
	return abs(ent->posx - ent->posPokex) + abs(ent->posy - ent->posPokey);
}

bool esMasCerca(entrenador *cerca, entrenador *lejos)
				  	  {
					  if(cerca->flagLeAsignaronPokenest && lejos->flagLeAsignaronPokenest)
					  {
						  return calcularDistancia(cerca) < calcularDistancia(lejos);
					  }
					  else
					  {
						  if (cerca->flagLeAsignaronPokenest && !lejos->flagLeAsignaronPokenest)
						  {
							  return 0;
						  }
						  if (!cerca->flagLeAsignaronPokenest && lejos->flagLeAsignaronPokenest)
						  {
							  return 1;
						  }
						  return -1;
					  }
				  }

////////////////////////////////////////////////////////////////////////////////////////////////////////
void enviarHeader(int unSocket, int unHeader) {
	char *recepcion = malloc(sizeof(int));
	memcpy(recepcion, &unHeader, sizeof(int));
	send(unSocket, recepcion, sizeof(int), 0);
	free(recepcion);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
void imprimir_archivo(char *rutaDelArchivo) {
	int c;
	FILE *file;
	file = fopen(rutaDelArchivo, "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			putchar(c);
		fclose(file);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
char *txtAString(char *rutaDelArchivo) {
	char * buffer = 0;
	long length;
	FILE * f = fopen(rutaDelArchivo, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) {
			fread(buffer, 1, length, f);
		}
		fclose(f);
	}
	return buffer;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
void notificarCaida() {
	int i;
	for (i = 0; i <= 1024; i++) {
		enviarHeader(clientesActivos[i].socket, 9);
	}
	printf("\n");
	printf(
			"AVISO: cierre inesperando del sistema. Notificando a los clientes y finalizando...\n");
	exit(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void terminarMapa() {
	list_destroy_and_destroy_elements(pokenests, (void*) free);
	list_destroy_and_destroy_elements(disponibles, (void*) free);
	int auxie;
	void destruir(bloq* self) {
		queue_destroy(self->colabloq);
		free(self);
	}
	list_destroy(listaContenedora);
	list_destroy(entrenadoresEnCurso);
	list_destroy(pokemons);
	free(datosMapa);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sumarRecurso2(t_list* items, char id) {
	ITEM_NIVEL* item = _search_item_by_id(items, id);

	if (item != NULL) {
		item->quantity = item->quantity >= 0 ? item->quantity + 1 : 0;
	} else {
		printf("WARN: Item %c no existente\n", id);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void matar(entrenador* entreni) {


	free(entreni->pokePeleador);
	log_info(logs,"libera pokemon (si se le asigno para el deadlock) del entrenador %c",entreni->simbolo);
	list_destroy_and_destroy_elements(entreni->asignados, (void*) free);
	log_info(logs, "libera lista de asignados de %c", entreni->simbolo);
	list_destroy_and_destroy_elements(entreni->solicitud, (void*) free);
	log_info(logs, "libera lista de solicitud de %c", entreni->simbolo);
	queue_destroy(entreni->colaAccion);
	free(entreni);

	}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void atenderConexion(void *numeroCliente) {
	int unCliente = *((int *) numeroCliente);
	char paquete[1024];
	char cambio[2];
	int status = 1;
	int estaLista = 0;

	entrenador* ent1 = malloc(sizeof(entrenador));

	t_log* logi;
	//remove("Mapi.log");

	logi = log_create("Mapi.log", "Mapa", false, log_level_from_string("INFO"));


	while (status != 0) {

		status = recv(clientesActivos[unCliente].socket, &paquete, sizeof(char),MSG_WAITALL);
		if (status != 0) {


			cambio[0] = paquete[0];

			if (isalpha(cambio[0]) || cambio[0] == '2' || cambio[0] == '6' || cambio[0] == '8' || cambio[0] == '4' || cambio[0] == '9') {

				status = recv(clientesActivos[unCliente].socket, &paquete, sizeof(char), MSG_WAITALL);

				cambio[1] = paquete[0];

				if (!estaLista) {

					pthread_mutex_lock(&mutexPaqueton);
					ent1->simbolo = cambio[1]; //almaceno simbolo en entrenador (paqueton [0] es variable global
					ent1->numeroCliente = unCliente; //numero de cliente para envio de informacion
					ent1->flagLeAsignaronPokenest = 0;
					ent1->numeroLlegada = (clientesActivos[unCliente].cliente
							- 1); //numero del entrenador
					estaLista = 1;
					ent1->estaMarcado = 0;
					ent1->fallecio = 0;
					ent1->pokePeleador = malloc(sizeof(metaDataPokemon));
					ent1->posx = 1; //posicion en x inicializada en 0
					ent1->posy = 1; // idem en y
					ent1->asignados = list_create();
					ent1->solicitud = list_create();
					ent1->pokemones = list_create();
					ent1->sumo = 0;
					int ka;
					for (ka = 0; ka < list_size(pokenests); ka++) {
						metaDataPokeNest* datosPo;
						datosPo = (metaDataPokeNest*) list_get(pokenests, ka);
						tabla* t = malloc(sizeof(tabla));
						tabla* otre = malloc(sizeof(tabla));
						t->pokenest = datosPo->caracterPokeNest[0];
						t->valor = 0;
						//log_info(logs,"Asignados: %c%d",t->pokenest,t->valor);
						otre->pokenest = datosPo->caracterPokeNest[0];
						otre->valor = 0;
						//	log_info(logs,"Solicitud: %c%d",otre->pokenest,otre->valor);
						list_add(ent1->asignados, t);
						list_add(ent1->solicitud, otre);
					}

					ent1->colaAccion = queue_create();

					queue_push(ent1->colaAccion, cambio[0]); // meto la accion del entrenador en la cola

					//list_replace(listaDeColasAccion, ent1->numeroLlegada, coli); // agrego en la lista que contiene su cola de accion

					//list_add(listaDeColasAccion, coli);

					//list_add_in_index(listaDeColasAccion,ent1->numeroLlegada,coli);

					nivel_gui_dibujar(items, nombreMapa);


					CrearPersonaje(items, ent1->simbolo, ent1->posx,
							ent1->posy); //mete al pj en el mapa

					list_add(entrenadoresEnCurso, ent1);

					usleep(1000);

					list_add(colaDeListosImp, ent1);
					queue_push(colaListos, ent1); //llego un entrenador entonces lo meto en la cola de listos

					log_info(logs, "entrenador %c a listos", ent1->simbolo); //informo por archivo de log la llegada del entrenad
					log_info(logs, "entrenadores en esa cola:");
					int auxilie;
					for(auxilie=0;auxilie<list_size(colaDeListosImp);auxilie++){
						entrenador* entprint;
						entprint = list_get(colaDeListosImp,auxilie);
						log_info(logs, "entrenador %c",entprint->simbolo);
					}

					sem_post(&sem_Listos); //produce un ent en colaListos




					pthread_mutex_unlock(&mutexPaqueton);

					//	aux = '\0';

				}

				//si el entrenador se encontraba registrado
				else {

					pthread_mutex_lock(&mupaq);
					queue_push(ent1->colaAccion, cambio[0]); //pusheo nuevo accionar a la cola auxiliar
					pthread_mutex_unlock(&mupaq);

				}

				//aux = '\0';
			}
			if (cambio[0] == '5') {
				pthread_mutex_lock(&muSem);

				int tamanioUno;
		   // 	log_info(logs,"llega un poke peleador de %c",ent1->simbolo);

				recv(clientesActivos[ent1->numeroCliente].socket, &tamanioUno,
						sizeof(int), MSG_WAITALL);

		//		log_info(logs, "el tamanio de la cosa uno es: %d", tamanioUno);
				/*	recv(clientesActivos[ent1->numeroCliente].socket, &tamanioDos,
				 sizeof(int), MSG_WAITALL);         */

				void *bufferCosaUno = malloc(tamanioUno +1);
				int nivel;

				recv(clientesActivos[ent1->numeroCliente].socket, bufferCosaUno,
						tamanioUno, MSG_WAITALL);
				recv(clientesActivos[ent1->numeroCliente].socket, &nivel,
						sizeof(int), MSG_WAITALL);

				((ent1->pokePeleador)->especie) = (char*) bufferCosaUno;
				((ent1->pokePeleador)->especie)[tamanioUno] = '\0';

				log_info(logs, "poke peleador de %c es %s", ent1->simbolo,(ent1->pokePeleador)->especie);
				((ent1->pokePeleador)->nivel) = nivel;
				log_info(logs, "nivel del poke peleador de %c es %d",ent1->simbolo, (ent1->pokePeleador)->nivel);

			//	free(bufferCosaUno);

				cambio[0]='\0';

				sem_post(&sem_llego);

				pthread_mutex_unlock(&muSem);
			}

			if (cambio[0] == 1) {
				status = 0;

			}

		}

	}
	pthread_mutex_lock(&mutexMurte);
	int auxiliar234;
		for (auxiliar234=0;auxiliar234<list_size(ent1->pokemones);auxiliar234++) {
			metaDataPokemon* pok;
			bloq* bli;
			tabla* d;

			pok = list_get(ent1->pokemones, auxiliar234);
		//	log_info(logs,"se libera al pokemon %s del entrenador %c EN LIBPOK",pok->especie,ent1->simbolo);
			bool esLaPokenest3(tabla* a) {
				return pok->especie[0] == a->pokenest;
			}
			d = list_find(disponibles, (void*) esLaPokenest3);
			d->valor++;
			sumarRecurso2(items, d->pokenest);
	    	nivel_gui_dibujar(items, nombreMapa);
			pok->estaOcupado = 0;

			bool esLad(bloq* ver) {
				return ver->pokenest == pok->especie[0];
			}


			int iiiuax;
			for (iiiuax = 0; iiiuax < list_size(disponibles); iiiuax++) {
				tabla* e;
				e = list_get(disponibles, iiiuax);
				log_info(logs, "Disponible de %c es %d", e->pokenest, e->valor);
			}



			bli = list_find(listaContenedora, (void*) esLad);

			sem_post(&(bli->sembloq));
			//sem_post(&(bli->sembloq));


			int vale;
			sem_getvalue(&(bli->sembloq),&vale);
			log_info(logs, "postea semaforo pokemon de %c y su valor es %d", bli->pokenest,vale);

		}
	ent1->fallecio = 1;
	queue_clean(ent1->colaAccion);
	BorrarItem(items, ent1->simbolo);
    nivel_gui_dibujar(items, nombreMapa);
	bool esEntrenador(entrenador* entiti) {
			return ent1->simbolo == entiti->simbolo;
		}

		list_remove_by_condition(entrenadoresEnCurso, (void*) esEntrenador);
		list_remove_by_condition(colaDeListosImp, (void*) esEntrenador);
		list_remove_by_condition(colaDeBloqImp, (void*) esEntrenador);

		int peo;
			for (peo = 0; peo < list_size(ent1->asignados); peo++) {
				tabla* a;
				tabla* b;
				a = list_get(ent1->asignados, peo);
				b = list_get(ent1->solicitud, peo);
				a->valor = 0;
				b->valor = 0;
			}


	list_destroy(ent1->pokemones);
    pthread_mutex_unlock(&mutexMurte);
	log_info(logi, "entrenador fallece");

}
