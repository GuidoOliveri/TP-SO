/*
 * libreriaPokedexServidor.h
 *
 *  Created on: 7/9/2016
 *      Author: utnso
 */

#ifndef LIBRERIAPOKEDEXSERVIDOR_H_
#define LIBRERIAPOKEDEXSERVIDOR_H_
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netdb.h>
#include <unistd.h>
#include "libSockets.h"
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include "osada.h"
//#include "TestServidor.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

t_log* logPS;

typedef struct{
	int cliente;
	int socket;
} t_infoCliente;

typedef char bloque[64];

t_infoCliente clientesActivos[1024];

typedef struct{
	void *buffer;
	int buffer_size;
}t_infoDirectorio;

typedef struct{
	int tipo_archivo;
	int size;
}t_getattr;


disco_osada miDisco;

pthread_mutex_t misMutex[2048];

pthread_mutex_t mutex_bloques;

t_log* log_Servidor;

/* Función para obtener el nombre de un archivo desde una tabla de archivos, en formato char * */
char *getFileName(unsigned char *nombreArchivo);

void iterarNombreAlReves(char* origen, unsigned char respuesta[17]);

char *obtenerNombre(char *unaRuta);

// Función para obtener el nombre de un directorio, dada su ruta
char *getNombreDirectorio(char *ruta);

/* Función para imprimir por pantalla el contenido de un archivo de texto */
void imprimir_archivo(char *rutaDelArchivo);

/* Función para volcar el contenido de un archivo .txt dentro de un string */
char *txtAString(char *rutaDelArchivo);

/* Función par enviar un código de operación (header) via sockets */
void enviarHeader(int unSocket, int unHeader);

/* Funciones para notificar a los clientes conectados de un cierre inesperado */
void enviarAvisoDeCierre();
void notificarCaida();

/* Función para atender una conexión en particular */
void atenderConexion(void *numeroCliente);

/* Serialización de strings */
void *serializarString(char *unString);

/* Concatenado de strings*/
char* concat(const char *s1, const char *s2);

/* Obtención de la cantidad de bloques a los que equivale un tamanio X*/
int calcularBloquesNecesarios(int tamanio);

/* Determinar si hay espacio en el disco para alojar N bloques*/
int hayBloquesLibres(int N);

/*conversión del contenido de un buffer en un string válido */
char *convertirString(void *buffer, int tamanio);

/* Búsqueda de la posicion de un un archivo/directorio en la tabla de archivos */
int recorrerDirectorio(char *nombre, int parentDir);

int buscarArchivo(char *unaRuta);

/* Búsqueda de la primera posición libre en la tabla de archivos*/
int buscarPosicionLibre();

int inicioDeDatos();

int inicioDeDatosEnBloques();

int hayLugarEnElUltimoBloque(int unTamanio);

int hayBloquesLibres(int unaCantidad);

int hayEspacioEnDisco(int tamanioActualArchivo, size_t tamanioAgregado, off_t offset);

int ultimoBloqueAsignado(int filePos);

/* Funciones de modificación del disco OSADA*/
void renombrar(char *nombre, int posicion);

void crearArchivo(char *nombreArchivo, int parentDir, int bloqueLibre, int posTablaArchivos);

void borrarArchivo(int posicion);

void crearDirectorio(char *nombreArchivo, char *ruta, int pos);

void borrarDirectorio(int posicion);

void chequearBitmap();

void actualizarTablaDeArchivos();

/* implementaciones de operaciones del filesystema osada */
t_getattr osada_getattr(char *unaRuta);

char *osada_readdir(char *unDirectorio);

int osada_open(char *unaRuta);

void *osada_read(char *unaRuta, int tamanioLectura, int offset);

int osada_create(char *ruta);

int osada_write(char *ruta, void *nuevoContenido, int sizeAgregado, int offset);

int osada_unlink(char *ruta);

int osada_mkdir(char *ruta);

int osada_rmdir(char *ruta);

int osada_rename(char *ruta, char *nuevoNombre);

int osada_truncate(char *ruta, int nuevoTamanio);

#endif /* LIBRERIAPOKEDEXSERVIDOR_H_ */
