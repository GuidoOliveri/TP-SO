/*
 ============================================================================
 Name        : PokedexServidor.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "PokedexServidor.h"
#include <pthread.h>

#define KNORMAL "\x1B[0m"
#define KROJO "\x1B[31m"
#define KVERDE "\x1B[32m"
#define KAMARILLO "\x1B[33m"
#define KAZUL "\x1B[34m"
#define KMAGENTA "\x1B[35m"
#define KCYAN "\x1B[36m"
#define KBLANCO "\x1B[37m"

#define BACKLOG 100
/* para testear sockets */
#define PACKAGESIZE 1024
#define PUERTO "7777"

t_log* logs;


void osada_iniciar(){

	// MAPEO EL DISCO
	int fd_disco;
	struct stat discoStat;
	fd_disco = open("/home/utnso/workspace/tp-2016-2c-Ni-Lo-Testeamos/PokedexServidor/challenge.bin", O_RDWR);
	fstat(fd_disco, &discoStat);
	miDisco.discoMapeado = mmap(0, discoStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_disco, 0);

	// CARGO EL HEADER
	miDisco.header = malloc(sizeof(osada_header));
	memcpy(miDisco.header, miDisco.discoMapeado, sizeof(osada_header));

	// CARGO LOS TAMAÑOS CORRESPONDIENTES A CADA ESTRUCTURA
	miDisco.cantBloques.bloques_header = 1;
		miDisco.cantBloques.bloques_bitmap = miDisco.header->bitmap_blocks;
		miDisco.cantBloques.bloques_tablaDeArchivos = 1024;

		int tablaAsignacionesEnBytes = (miDisco.header->fs_blocks - 1 -1024 - miDisco.header->bitmap_blocks)
		    		* 4;
		    div_t bloquesTablaAsignacion = div(tablaAsignacionesEnBytes, 64);

		if(bloquesTablaAsignacion.rem != 0){
		    miDisco.cantBloques.bloques_tablaDeAsignaciones = bloquesTablaAsignacion.quot + 1;
		}
		else{
			miDisco.cantBloques.bloques_tablaDeAsignaciones = bloquesTablaAsignacion.quot;
		}
		miDisco.cantBloques.bloques_datos = miDisco.header->fs_blocks - miDisco.header->bitmap_blocks
				- 1 - 1024 - miDisco.cantBloques.bloques_tablaDeAsignaciones;

	// CARGO EL BITMAP
	int inicioBitmap = 64;
	int tamanioBitmap = miDisco.header->fs_blocks / 8;
	char *miBitArray = malloc(tamanioBitmap);
	memcpy(miBitArray, &miDisco.discoMapeado[inicioBitmap], tamanioBitmap);
	miDisco.bitmap = bitarray_create(miBitArray, tamanioBitmap);

	// CARGO LA TABLA DE ARCHIVOS
	int inicioTablaArchivos = (1 + miDisco.header->bitmap_blocks) * 64;
	memcpy(miDisco.tablaDeArchivos, &miDisco.discoMapeado[inicioTablaArchivos],
			(2048 * sizeof(osada_file)));

	// CARGO LA TABLA DE ASIGNACIONES
	int inicioTablaAsignaciones = (1 + 1024 + miDisco.header->bitmap_blocks) * 64;
	int tamanioTablaDeAsignaciones = (miDisco.header->fs_blocks - 1 - 1024 -
			miDisco.header->bitmap_blocks) * 64;
	miDisco.tablaDeAsignaciones = malloc(tamanioTablaDeAsignaciones);
	memcpy(miDisco.tablaDeAsignaciones, &miDisco.discoMapeado[inicioTablaAsignaciones], tamanioTablaDeAsignaciones);

}

int main(int argc, char **argv) {



	//LOGS
	remove("PokeServidor.log");
	puts("Creando archivo de logueo PokeServidor...\n");
	logs = log_create("PokeServidor.log", "PokedexServidor", true, log_level_from_string("INFO"));
	puts("Log Pokedex Servidor creado exitosamente \n");

	//Levanto el disco Osada
	 osada_iniciar();
	 /*
	 miDisco = osada_iniciar();
	 int fd_disco;
	 struct stat discoStat;
	 fd_disco = open("/home/utnso/workspace/tp-2016-2c-Ni-Lo-Testeamos/PokedexServidor/challenge.bin", O_RDWR);
	 fstat(fd_disco, &discoStat);
	 miDisco.discoMapeado = mmap(0, discoStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_disco, 0);
	*/

	 // Inicio los semáforos
	 int m;
	 for (m = 0; m < 2048; m++){
	 		 pthread_mutex_init(&misMutex[m], NULL);
	 	 }


	int socketEscucha, retornoPoll;
	int fd_index = 0;


	struct pollfd fileDescriptors[100];
	int cantfds = 0;
	socketEscucha = setup_listen("localhost", PUERTO);
	listen(socketEscucha, 1024);

	fileDescriptors[0].fd = socketEscucha;
	fileDescriptors[0].events = POLLIN;
	cantfds++;

	int enviar = 1;
	int cliente = 1;
	t_infoCliente unCliente;
	int n = 0;
	int *numeroCliente;
	pthread_t hiloAtenderConexiones[1024];


	while(enviar){

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
				listen(socketEscucha, BACKLOG);
				struct sockaddr_in addr;
				socklen_t addrlen = sizeof(addr);
				int socketCliente = accept(socketEscucha,
				(struct sockaddr *) &addr, &addrlen);


				unCliente.cliente = cliente;
				unCliente.socket = socketCliente;
				clientesActivos[n] = unCliente;
				int nroCliente = n++;
				numeroCliente = malloc(sizeof(int));
				numeroCliente = &nroCliente;

				pthread_create(&hiloAtenderConexiones[n], NULL, atenderConexion, numeroCliente);

				cliente++;
				n++;

				fileDescriptors[cantfds].fd = socketCliente;
				fileDescriptors[cantfds].events = POLLIN;
				cantfds++;

				goto llamadaPoll;
			}
		}
	}

close(socketEscucha);


return 0;

}

