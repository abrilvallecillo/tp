#ifndef _MEMORIA_
#define _MEMORIA_
#include <utils/hilos.h>
#include <commons/bitarray.h>

extern lista_sincronizada* codigos_programas;
extern lista_sincronizada* tablaPaginasProcesos;
extern void* memoria;
extern pthread_mutex_t* mutexMemoria;
extern int cantidadMarcos;
extern t_bitarray * marcosLibres;
extern pthread_mutex_t* mutexMarcosLibres;
extern void * tamanioMarcosLibres;

lista_sincronizada* codigos_programas;
lista_sincronizada* tablaPaginasProcesos;
void* memoria;
pthread_mutex_t* mutexMemoria;
int cantidadMarcos;
t_bitarray * marcosLibres;
pthread_mutex_t* mutexMarcosLibres;
int pidAComparar;
int nroPaginaAComparar;
void * tamanioMarcosLibres;

typedef struct{
    int pid;
    t_list* listaPaginas;
} t_tablaPaginas;

typedef struct{
    int numeroPagina;
    int numeroMarco;
    bool validez;
}fila_tabla_de_paginas;

bool encontrarTablaPorPid(void * elemento);
bool igualNumeroDePagina (void * elemento);
int contarMarcosLibres();
int cantidadTablaPaginas(int pid);
t_tablaPaginas * buscarTablaDePaginas(int pid);
fila_tabla_de_paginas * buscarUltimaFila(t_tablaPaginas * tabla);
int encontrarPrimerMarcoLibre();
fila_tabla_de_paginas * buscarUltimaFilaValida(t_tablaPaginas * tabla);
bool filaValida(void * elemento);
#endif