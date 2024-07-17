#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <utils/hilos.h>
#include <utils/conexiones.h>
#include <commons/log.h>
#include <commons/config.h> 
#include "atencion_modulos.h"
#include "memoria.h"
#include "configuracion.h"
#include <utils/logger_concurrente.h>
#include <commons/string.h>

int main(int argc, char* argv[]) {

    codigos_programas = crearListaSincronizada();
    mutexMemoria = malloc(sizeof(pthread_mutex_t));
    mutexMarcosLibres = malloc(sizeof(pthread_mutex_t));
    crearMutex(mutexMemoria);
    crearMutex(mutexMarcosLibres);
    
    inicializarLogger("memoria.log", "Log de la Memoria");
    char * direccion_config;
    if(argc < 2) { direccion_config = string_duplicate("memoria.config");
    } else { direccion_config = string_duplicate(argv[1]);
    } obtener_config(direccion_config);
    
    int conexion_servidor = crearConexionServidor(configuracion.PUERTO);
    //log_info(logger, "Servidor listo para recibir al cliente");
    
    cantidadMarcos = (int) (configuracion.TAMANIO / configuracion.TAMANIO_PAGINAS);
    tamanioMarcosLibres = malloc(cantidadMarcos/8);
    marcosLibres = bitarray_create_with_mode(tamanioMarcosLibres, cantidadMarcos/8, LSB_FIRST);
    memoria = malloc(configuracion.TAMANIO);
    for(int i = 0; i< bitarray_get_max_bit(marcosLibres); i++) { bitarray_set_bit(marcosLibres, i); }
    tablaPaginasProcesos = crearListaSincronizada();
    
    while (1) {
        int * socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperarCliente(conexion_servidor);
        
        if(*socket_cliente < 0) {
            free(socket_cliente);
            break;
        }
        pthread_t hiloAtencion = crearHilo(atenderPeticiones, socket_cliente);
        pthread_detach(hiloAtencion);
    }

    eliminarLista(codigos_programas);
    free(direccion_config);
    free(tamanioMarcosLibres);
    bitarray_destroy(marcosLibres);
    pthread_mutex_destroy(mutexMemoria);
    pthread_mutex_destroy(mutexMarcosLibres);
    eliminarLista(tablaPaginasProcesos);
    free(mutexMemoria);
    free(mutexMarcosLibres);
    free(memoria);
    destruirLogger();
    return 0;
}

