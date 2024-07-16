#ifndef UTILS_HILOS_H_
#define UTILS_HILOS_H_

#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <stdlib.h>

typedef struct {
    int entero;
    pthread_mutex_t * mutex_entero;
} entero_sincronizado;

typedef struct {
    t_list * lista;
    pthread_mutex_t * mutex_lista;
} lista_sincronizada;


pthread_t crearHilo(void * (*funcionDeHilo)(void *), void * parametroDeFuncion);

void crearMutex(pthread_mutex_t * mutex);

entero_sincronizado * crearEnteroSincronizado();
void incrementarEnteroSincronizado(entero_sincronizado * entero_sincro);
void disminuirEnteroSincronizado(entero_sincronizado * entero_sincro);
void modificarEnteroSincronizado(entero_sincronizado * entero_sincro, int valor);
int leerEnteroSincronizado(entero_sincronizado * entero_sincro);
sem_t * crearSemaforo(int valor);
void eliminarSemaforo(sem_t * semaforo);

lista_sincronizada * crearListaSincronizada();
void * leerElementoListaSincronizadaSegun(lista_sincronizada * lista_sinc, bool (*criterio)(void *));
void agregarElementoAListaSincronizada(lista_sincronizada * lista_sinc, void * elemento);
bool existeElementoEnListaSincronizada(lista_sincronizada * lista_sinc, bool (*criterio) (void *));
void eliminarLista(lista_sincronizada * lista_sinc);
#endif