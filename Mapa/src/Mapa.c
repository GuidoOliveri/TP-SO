/*
 ============================================================================
 Name        : Mapa.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <nivel.h>
#include <tad_items.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include "libSockets.h"
#include <curses.h>
#include <string.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include "libreriaMapa.h"

#define PACKAGESIZE 1024

//variables globales
char paqueton[10] =
		{ '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
extern metaDataPokeNest *datos;
extern int numEntrenador;
extern t_infoCliente clientesActivos[1024];
char* nombreMapa;

//variables del mapa
int nE = 0; //numero entrenador
metaDataComun* datosMapa;
metaDataPokeNest* datosPokenest;
metaDataPokemon* datosPokemon;
int rows; // nro de filas
int cols; // nro de columnas

//log
t_log* logs;

//listas
t_list* pokenests;
t_list* items;
t_list* listaDeColasAccion;

//colas
t_queue* colaListos;
t_queue* colaBloqueados;
t_queue* colaAccion;

//semaforos
sem_t sem_Nuevos;
sem_t sem_Listos;
sem_t sem_Bloqueados;
sem_t sem_ListaColas;
sem_t sem_quantum;

//declara hilos
pthread_t hiloDePlanificador;
pthread_t hiloAtenderConexiones[1024];

//arranque de planificacion
void planificador(void* argu) {

	char* argument = (char*) argu;

	int q = datosMapa->quantum;

	while (1) {
		sem_wait(&sem_Listos); //semaforo de nuevos bloqueando que se saque un ent si la cola esta vacia


		log_info(logs,"volvio a empezar el while, sigo re loco, seguro ya se camnio el paqueton: %d",paqueton[1]);

		//caso roun robin
		if (!strcmp(datosMapa->algoritmo, "RR")) {


			int acto;
			t_queue *colaAction;
			entrenador* ent1;

			log_info(logs,"hasta aca no rompe");


			ent1 = (entrenador*) queue_pop(colaListos);
			log_info(logs,"funca");



			colaAction = list_get(listaDeColasAccion, ent1->numeroLlegada); //saco cola de accion de la lista de entrenadores
			log_info(logs,"funca2");


			log_info(logs,"hasta aca no rompe parte 2");


			while (q) {



				acto = (int) queue_pop(colaAction);
				log_info(logs,"funca3");


				if (isalpha(acto)) {
					int ka;
					for (ka = 0; ka < list_size(pokenests); ka++) {
						datosPokenest = (metaDataPokeNest*) list_get(pokenests,
								ka);
						if (datosPokenest->caracterPokeNest[0] == acto) {

							log_info(logs, "antes del send %s",
									datosPokenest->posicion);
							int pedo;
							pedo = send((clientesActivos[ent1->numeroCliente]).socket,
									datosPokenest->posicion, 5, 0);

							log_info(logs, "Se envio coordenadas: %d",pedo);
							ka = list_size(pokenests);

						}
					}

					q--;

				}


				log_info(logs,"paso el isalpha");

				sem_wait(&sem_quantum);
				//8 es 56, 2 es 50, 4 es 52, 6 es 54

				if (isdigit(acto)) {
					log_info(logs,"se metio al isdigit");
					if (acto == 50 || acto == 52 || acto == 54 || acto == 56) {

						sleep(1);
						switch (acto) {

						case 56:
							if (ent1->posy > 1) {
                               log_info(logs,"mueva arriba");

								ent1->posy--;
								MoverPersonaje(items, ent1->simbolo, ent1->posx, ent1->posy);
																nivel_gui_dibujar(items, argument);
								q--;
							}
							break;

						case 50:
							if (ent1->posy < rows) {
                               log_info(logs,"mueva abajo");

								ent1->posy++;
								MoverPersonaje(items, ent1->simbolo, ent1->posx, ent1->posy);
																nivel_gui_dibujar(items, argument);
								q--;
							}
							break;

						case 52:
							if (ent1->posx > 1) {
                               log_info(logs,"mueva izquierda");

								ent1->posx--;
								MoverPersonaje(items, ent1->simbolo, ent1->posx, ent1->posy);
																nivel_gui_dibujar(items, argument);
								q--;
							}
							break;
						case 54:
							if (ent1->posx < cols) {
                               log_info(logs,"mueva derecha");

								ent1->posx++;
								MoverPersonaje(items, ent1->simbolo, ent1->posx, ent1->posy);
								nivel_gui_dibujar(items, argument);

								q--;
							}
							break;





						}
					}

					if (acto == 9) {
						queue_push(colaBloqueados, ent1);
						q = datosMapa->quantum;
					}

				}
			}


			log_info(logs,"ahora pase por aca papurri, sigo andando");
			/* if (   ((p == 26) && (q == 10)) || ((x == 26) && (y == 10)) ) {
			 restarRecurso(items, 'H');
			 }
			 if (   ((p == 19) && (q == 9)) || ((x == 19) && (y == 9)) ) {
			 restarRecurso(items, 'F');
			 }
			 if (   ((p == 8) && (q == 15)) || ((x == 8) && (y == 15)) ) {
			 restarRecurso(items, 'M');
			 } */

	        if (q == 0){
	        	queue_push(colaListos,ent1);
	        	q = datosMapa->quantum;
	        	sem_post(&sem_Listos);
	        }


		}
	}
}



///////////////////////////////////////////Main//////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {

	//nombre de mapa
	nombreMapa = argv[1];
	//inicializo listas
	pokenests = list_create();
	items = list_create();
	listaDeColasAccion = list_create();
	//inicializo colas
	colaListos = queue_create();
	colaBloqueados = queue_create();
	colaAccion = queue_create();
	//inicializo semaforos
	sem_init(&sem_Nuevos, 0, 0);
	sem_init(&sem_Listos, 0, 0);
	sem_init(&sem_Bloqueados, 0, 0);
	sem_init(&sem_ListaColas, 0, 0);
	sem_init(&sem_quantum, 0, 0);

	remove("Mapa.log");
	logs = log_create("Mapa.log", "Mapa", false, log_level_from_string("INFO"));

// CONFIG

	datosMapa = malloc(sizeof(metaDataComun));
	datosPokenest = malloc(sizeof(metaDataPokeNest));
	datosPokemon = malloc(sizeof(metaDataPokemon));

	char* configMetaMapa = string_from_format("%s/Mapas/%s/metadata", argv[2],
			argv[1]);
	if (!leerConfiguracion(configMetaMapa, &datosMapa)) {
		log_error(logs,
				"Error al leer el archivo de configuracion de Metadata\n");
		return 1;
	}

//por ahora
	char* configPokenest = string_from_format("%s/Mapas/%s/PokeNests", argv[2],
			argv[1]);
	if (!leerConfigPokenest(configPokenest, pokenests)) {
		log_error(logs,
				"Error al leer el archivo de configuracion de Metadata Pokenest\n");
		return 2;
	}

	char* configPoke = string_from_format(
			"%s/Mapas/%s/PokeNests/Pikachu/Pikachu001.dat", argv[2], argv[1]);
	if (!leerConfigPokemon(configPoke, &datosPokemon)) {
		log_error(logs,
				"Error al leer el archivo de configuracion de Metadata de Pokemons\n");
		return 3;
	}

	log_info(logs,
			"Los tres archivos de config fueron creados exitosamente!\n");


	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);


	//POKENEST
		int ka;

		for (ka = 0; ka < list_size(pokenests); ka++) {
			datosPokenest = (metaDataPokeNest*) list_get(pokenests, ka);
			char** posPoke;
			posPoke = string_split(datosPokenest->posicion, ";");
			char ide;
			ide = datosPokenest->caracterPokeNest[0];

			CrearCaja(items, ide, atoi(posPoke[0]), atoi(posPoke[1]),
					datosPokenest->cantPokemons);
		}


	nivel_gui_dibujar(items, argv[1]);

	//hilo de planificacion

	pthread_create(&hiloDePlanificador, NULL, (void*)planificador, (void*)argv[1]);


//SOCKETS
	log_info(logs,
			"iniciado el servidor principal del Mapa. Aguardando conexiones...\n\n");

	signal(SIGINT, notificarCaida);

	int socketEscucha, retornoPoll;
	int fd_index = 0;

	struct pollfd fileDescriptors[100];
	int cantfds = 0;

	char* puertoChar = string_itoa(datosMapa->puerto);
	socketEscucha = setup_listen(datosMapa->ip, puertoChar);

	listen(socketEscucha, 1024);

	fileDescriptors[0].fd = socketEscucha;
	fileDescriptors[0].events = POLLIN;
	cantfds++;

	int enviar = 1;
	int cliente = 1;
	t_infoCliente *infoCliente;
	t_infoCliente unCliente;
	int n = 0;
	int *numeroCliente;


	//conexion
	while (enviar) {

		llamadaPoll:

		// Inicio la función poll()
		retornoPoll = poll(fileDescriptors, cantfds, -1);

		// valido que haya iniciado bien
		if (retornoPoll == -1) {
			printf("Error en la funcion poll\n");
		}

		// Recorro la lista de file descriptors chequeando si el poll() retornó por una modificación.
		// De ser así, acepto la conexión, delego la atención del socket
		// a un hilo y vuelvo para arriba.

		for (fd_index = 0; fd_index < cantfds; fd_index++) {
			if (fileDescriptors[fd_index].fd == socketEscucha) {
				listen(socketEscucha, 100);
				struct sockaddr_in addr;
				socklen_t addrlen = sizeof(addr);
				int socketCliente = accept(socketEscucha,
						(struct sockaddr *) &addr, &addrlen);

				infoCliente = malloc(sizeof(t_infoCliente));
				infoCliente->cliente = cliente;
				infoCliente->socket = socketCliente;

				unCliente.cliente = cliente;
				unCliente.socket = socketCliente;
				clientesActivos[n] = unCliente;
				int nroCliente = n++;
				numeroCliente = malloc(sizeof(int));
				numeroCliente = &nroCliente;

				//    pthread_create(&hiloImprimirGiladas[n],NULL, imprimirGiladas, infoCliente);
				pthread_create(&hiloAtenderConexiones[n], NULL,
						(void*) atenderConexion, numeroCliente);

				cliente++;
				n++;

				fileDescriptors[cantfds].fd = socketCliente;
				fileDescriptors[cantfds].events = POLLIN;
				cantfds++;

				goto llamadaPoll;
			}
		}
	}





	//liberamos las listas y toda la ganzada

	for (ka = 0; ka < list_size(pokenests); ka++) {
		datosPokenest = (metaDataPokeNest*) list_get(pokenests, ka);
		char ide;
		ide = datosPokenest->caracterPokeNest[0];
		BorrarItem(items, ide);
	}

	nivel_gui_terminar();

	free(datos); //siendo datos una variable global para el almacenamiento de pokenest
	free(datosMapa);
	free(datosPokenest);
	free(datosPokemon);
	/*	list_destroy_and_destroy_elements(pokenests,free);
	 list_destroy_and_destroy_elements(items,free);
	 list_destroy_and_destroy_elements(listaDeColasAccion,free);
	 queue_destroy_and_destroy_elements(colaListos,free);
	 queue_destroy_and_destroy_elements(colaBloqueados,free);
	 */
	return EXIT_SUCCESS;

}
