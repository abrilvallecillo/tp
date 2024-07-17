#include "memoria.h"


int contarMarcosLibres(){
    int contador = 0;
    for(int i = 0; i< bitarray_get_max_bit(marcosLibres); i++){
        contador += bitarray_test_bit(marcosLibres, i);
    }
    return contador;
}

int cantidadTablaPaginas(int pid){
    t_tablaPaginas * tabla = buscarTablaDePaginas(pid);
    if (list_size(tabla->listaPaginas) > 0){ return list_count_satisfying(tabla->listaPaginas, filaValida);   
    } return list_size(tabla->listaPaginas);
}

t_tablaPaginas * buscarTablaDePaginas(int pid){
    pthread_mutex_lock(tablaPaginasProcesos->mutex_lista);
    pidAComparar = pid;
    t_tablaPaginas * tabla = list_find(tablaPaginasProcesos->lista, encontrarTablaPorPid);
    pthread_mutex_unlock(tablaPaginasProcesos->mutex_lista);
    return tabla;
}

bool encontrarTablaPorPid(void * elemento){
    t_tablaPaginas * tabla = (t_tablaPaginas *) elemento;
    return tabla->pid == pidAComparar;
}

fila_tabla_de_paginas * buscarUltimaFila(t_tablaPaginas * tabla){
    return (fila_tabla_de_paginas*) list_get(tabla->listaPaginas, list_size(tabla->listaPaginas)-1);
}

int encontrarPrimerMarcoLibre(){
    for(int i = 0; i<bitarray_get_max_bit(marcosLibres); i++){
        if(bitarray_test_bit(marcosLibres, i)){
            return i;
        }
    }
    return -1; //Para evitar un warning innecesario
}

fila_tabla_de_paginas * buscarUltimaFilaValida(t_tablaPaginas * tabla){
    t_list * lista = list_duplicate(tabla->listaPaginas);
    fila_tabla_de_paginas * ultimaFilaValida;
    do
    {
        int indice_lista = list_size(lista)-1;
        if(indice_lista < 0) {
            list_destroy(lista);
            return NULL;
        }
        ultimaFilaValida = (fila_tabla_de_paginas*) list_get(lista, indice_lista);
        if(!ultimaFilaValida) {
            list_destroy(lista);
            return NULL;
        }
        list_remove_element(lista, ultimaFilaValida);
    }while (!ultimaFilaValida->validez);
    list_destroy(lista);
    return ultimaFilaValida;
}

bool igualNumeroDePagina (void * elemento){
    fila_tabla_de_paginas * fila = (fila_tabla_de_paginas*) elemento;
    return fila->numeroPagina != nroPaginaAComparar;
}

bool filaValida(void * elemento) {
    fila_tabla_de_paginas * fila = (fila_tabla_de_paginas *) elemento;
    return fila->validez; 
}