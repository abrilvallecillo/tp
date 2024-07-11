#include "hilos.h"
#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>

pthread_t crearHilo(void * (*funcionDeHilo)(void *), void * parametroDeFuncion) {
    pthread_t hilo;
    int seCreoBien = pthread_create(&hilo, NULL, funcionDeHilo, parametroDeFuncion);
    if(seCreoBien) {
        fprintf(stderr, "Hubo un error al crear el Hilo");
        abort();
    }
    return hilo;
}

void crearMutex(pthread_mutex_t * mutex) {
    int mutex_mal_creado = pthread_mutex_init(mutex, NULL);
    if(mutex_mal_creado) {
        fprintf(stderr, "Error al crear el mutex");
        abort();
    }
}

entero_sincronizado * crearEnteroSincronizado() { //Se necesita pedir memoria para el mutex y para el entero_sincro
    entero_sincronizado * entero_sincro = malloc(sizeof(entero_sincronizado));
    entero_sincro->entero = 0;
    entero_sincro->mutex_entero = malloc(sizeof(pthread_mutex_t));
    crearMutex(entero_sincro->mutex_entero);
    return entero_sincro;
}

void incrementarEnteroSincronizado(entero_sincronizado * entero_sincro) {
    pthread_mutex_lock(entero_sincro->mutex_entero);
    entero_sincro->entero++;
    pthread_mutex_unlock(entero_sincro->mutex_entero);
}

void disminuirEnteroSincronizado(entero_sincronizado * entero_sincro) {
    pthread_mutex_lock(entero_sincro->mutex_entero);
    entero_sincro->entero--;
    pthread_mutex_unlock(entero_sincro->mutex_entero);
}

void modificarEnteroSincronizado(entero_sincronizado * entero_sincro, int valor){
    pthread_mutex_lock(entero_sincro->mutex_entero);
    entero_sincro->entero = valor;
    pthread_mutex_unlock(entero_sincro->mutex_entero);
}

int leerEnteroSincronizado(entero_sincronizado * entero_sincro) {
    int valor;
    pthread_mutex_lock(entero_sincro->mutex_entero);
    valor = entero_sincro->entero;
    pthread_mutex_unlock(entero_sincro->mutex_entero);
    return valor;
}

sem_t * crearSemaforo(int valor) {
    sem_t * semaforo = malloc(sizeof(sem_t));
    sem_init(semaforo, 0, valor);
    return semaforo; 
}

void eliminarSemaforo(sem_t * semaforo) {
    sem_destroy(semaforo);
    free(semaforo);
}

lista_sincronizada * crearListaSincronizada() {
    lista_sincronizada * lista_sinc = malloc(sizeof(lista_sincronizada));
    lista_sinc->lista = list_create();
    lista_sinc->mutex_lista = malloc(sizeof(pthread_mutex_t));
    crearMutex(lista_sinc->mutex_lista);
    return lista_sinc;
}


void * leerElementoListaSincronizadaSegun(lista_sincronizada * lista_sinc, bool (*criterio)(void *)) {
    pthread_mutex_lock(lista_sinc->mutex_lista);
    void * valor = list_find(lista_sinc->lista, criterio);
    pthread_mutex_unlock(lista_sinc->mutex_lista);
    return valor;
}

void eliminarLista(lista_sincronizada * lista_sinc) {
    list_destroy(lista_sinc->lista);
    pthread_mutex_destroy(lista_sinc->mutex_lista);
    free(lista_sinc->mutex_lista);
    free(lista_sinc);
}

void agregarElementoAListaSincronizada(lista_sincronizada * lista_sinc, void * elemento) {
    pthread_mutex_lock(lista_sinc->mutex_lista);
    list_add(lista_sinc->lista, elemento);
    pthread_mutex_unlock(lista_sinc->mutex_lista);
}

bool existeElementoEnListaSincronizada(lista_sincronizada * lista_sinc, bool (*criterio) (void *)) {
    pthread_mutex_lock(lista_sinc->mutex_lista);
    bool resultado = list_any_satisfy(lista_sinc->lista, criterio);
    pthread_mutex_unlock(lista_sinc->mutex_lista);
    return resultado;
}