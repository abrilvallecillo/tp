#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <utils/hilos.h>
#include <utils/conexiones.h>
#include <commons/log.h>
#include <commons/config.h> 
#include <pthread.h>
#include "atencion_entrada_salida.h"
#include "consola.h"
#include "control_interrupciones.h"
#include "manejo_memoria.h"
#include "planificacion.h"
#include "kernel.h"
#include <utils/logger_concurrente.h>
#include "configuracion.h"
#include <commons/string.h>
#include <string.h>

int main(int argc, char* argv[]) {

    
    //Inicializamos las colas, el logger y el config
    cola_memoria = crearCola();
    cola_new = crearCola();
    cola_ready = crearCola();
    cola_ready_vrr = crearCola();
    cola_exit = crearCola();
    inicializarLogger("Log del Kernel", "kernel.log");
    solicitaron_finalizar_proceso = crearSemaforo(0);
    no_surgio_otra_interrupcion = crearSemaforo(0);
    hay_peticion_en_cola = crearSemaforo(0);
    hay_peticiones_memoria = crearSemaforo(0);
    hay_que_parar_planificacion = crearSemaforo(1);
    hay_procesos_cola_exit = crearSemaforo(0);
    hay_procesos_cola_new = crearSemaforo(0);
    hay_procesos_cola_ready = crearSemaforo(0);
    pid_nuevo_proceso = 0;
    lista_interfaces = list_create();
    lista_recursos = list_create();
    lista_estados = list_create();
    lista_mutex_eliminacion_interfaz = crearListaSincronizada();
    pid_fin_usuario = crearEnteroSincronizado();
    modificarEnteroSincronizado(pid_fin_usuario, -1);
    mutex_pid_comparar = malloc(sizeof(pthread_mutex_t));
    mutex_nombre_recurso = malloc(sizeof(pthread_mutex_t));
    mutex_lista_interfaces = malloc(sizeof(pthread_mutex_t));
    crearMutex(mutex_pid_comparar);
    crearMutex(mutex_nombre_recurso);
    crearMutex(mutex_lista_interfaces);
    char * direccion_config;
    if(argc < 2) {
        direccion_config = string_duplicate("kernel.config");
    } else {
        direccion_config = string_duplicate(argv[1]);
    }
    obtener_config(direccion_config);
    algoritmo_elegido = algoritmoPorString(configuracion.ALGORITMO_PLANIFICACION);
    pid_actual = crearEnteroSincronizado();
    modificarEnteroSincronizado(pid_actual, -1);
    
    grado_multiprogramacion = crearEnteroSincronizado();
    modificarEnteroSincronizado(grado_multiprogramacion, configuracion.GRADO_MULTIPROGRAMACION);
    cargarRecursos();
    diferencia_cambio_mp = crearEnteroSincronizado();
    grado_multiprogramacion_habilita = crearSemaforo(grado_multiprogramacion->entero);
    quantum_actual = crearEnteroSincronizado();
    

    //Creamos los hilos para las conexiones del kernel como cliente y la consola
    pthread_t  planificador_de_procesos_h = crearHilo(planificarProcesos, NULL);
    pthread_t  control_de_interrupciones_h = crearHilo(interrupcionDeProcesos, NULL);
    pthread_t  manejo_de_memoria_h = crearHilo(manejarMemoria, NULL);
    pthread_t  consola_h = crearHilo(consola, NULL);

    pthread_detach(consola_h);
    pthread_detach(planificador_de_procesos_h);
    pthread_detach(control_de_interrupciones_h);
    pthread_detach(manejo_de_memoria_h);

    //Creamos la conexión para que recibamos las conexiones de I/O
    int conexion_servidor_io = crearConexionServidor(configuracion.PUERTO_ESCUCHA);

    //Hacemos el loop para ir recibiendo conexiones de I/O
    while (1){
        int * socket_cliente_io = malloc(sizeof(int));
        *socket_cliente_io = esperarCliente(conexion_servidor_io);
        if(*socket_cliente_io < 0) {
            free(socket_cliente_io);
            break;
        }
        pthread_t hiloAtencionIO = crearHilo(atenderIO, socket_cliente_io);
        pthread_detach(hiloAtencionIO);
    }
   
    free(direccion_config);
    eliminarSemaforo(hay_peticion_en_cola);
    eliminarSemaforo(hay_peticiones_memoria);
    eliminarSemaforo(no_surgio_otra_interrupcion);
    eliminarSemaforo(solicitaron_finalizar_proceso);
    eliminarSemaforo(grado_multiprogramacion_habilita);
    pthread_mutex_destroy(mutex_lista_interfaces);
    pthread_mutex_destroy(mutex_nombre_recurso);
    pthread_mutex_destroy(mutex_pid_comparar);
    free(mutex_lista_interfaces);
    free(mutex_nombre_recurso);
    free(mutex_pid_comparar);
    
    return 0;
}


