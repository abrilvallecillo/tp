#include "manejo_memoria.h"
#include <utils/conexiones.h>
#include <utils/procesos.h>
#include <commons/collections/queue.h>
#include "kernel.h"
#include <commons/log.h>
#include "configuracion.h"
#include <semaphore.h>
#include <utils/logger_concurrente.h>
#include <commons/string.h>

pcb * recibirRespuestaACrearProceso(int conexion_memoria);
inicializar_proceso * crearInicializarProceso(peticion_memoria * nueva_peticion);
t_buffer * serializarBorrarMemoriaP(peticion_memoria * nueva_peticion);
void cargarProcesoEnSistema(uint32_t pid);

void *manejarMemoria(void * sin_parametro) {
    int conexion_memoria = crearConexionCliente(configuracion.IP_MEMORIA, configuracion.PUERTO_MEMORIA);
    int handshake_memoria = enviarHandshake(conexion_memoria, KERNEL);
    if(handshake_memoria == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con la memoria");
        abort();
    }
    
    while(1){
        sem_wait(hay_peticiones_memoria);
        peticion_memoria * nueva_peticion = sacarDeCola(cola_memoria);
        t_buffer * buffer;
        switch(nueva_peticion->operacion) {
            case CREAR_PROCESO:
                inicializar_proceso * nueva_inicializacion = crearInicializarProceso(nueva_peticion);
                buffer = serializarInicializarProceso(nueva_inicializacion);
                enviarBufferProcesoConMotivo(buffer, CREAR_PROCESO, conexion_memoria);
                pcb * pcb_obtenido = recibirRespuestaACrearProceso(conexion_memoria);
                if(pcb_obtenido == NULL) {
                    free(nueva_inicializacion->direccion_codigo);
                    free(nueva_inicializacion);
                    break;
                }
                cargarProcesoEnSistema(pcb_obtenido->PID);
                agregarACola(cola_new, pcb_obtenido);
                char * mensajeCreacionProceso = string_from_format("Se crea el proceso %d en NEW", pcb_obtenido->PID);
                logInfoSincronizado(mensajeCreacionProceso);
                sem_post(hay_procesos_cola_new);
                free(mensajeCreacionProceso);
                free(nueva_inicializacion->direccion_codigo);
                free(nueva_inicializacion);
                break;
            case P_BORRAR_MEMORIA:
                buffer = serializarBorrarMemoriaP(nueva_peticion);
                enviarBufferProcesoConMotivo(buffer, P_BORRAR_MEMORIA, conexion_memoria);
                break;
            default:
                fprintf(stderr, "Error, peticion de memoria desconocida");

        }
        queue_destroy(nueva_peticion->cola_parametros);
        free(nueva_peticion);
    }
    return NULL;
}

pcb * recibirRespuestaACrearProceso(int conexion_memoria) {
    t_paquete * paquete_proceso = recibirPaqueteGeneral(conexion_memoria);
    if(paquete_proceso == NULL || paquete_proceso->codigoOperacion != PROCESO_CREADO) 
        return NULL;
    pcb * pcb_nuevo = deserializarProceso(paquete_proceso->buffer);
    eliminar_paquete(paquete_proceso);
    return pcb_nuevo;
}

 //Es una estructura lo que crea, que tiene los datos necesarios para enviarle a memoria que cree el proceso
inicializar_proceso * crearInicializarProceso(peticion_memoria * nueva_peticion) {
    inicializar_proceso * indicaciones_memoria = malloc(sizeof(inicializar_proceso));
    int * auxiliarValores = (int *) queue_pop(nueva_peticion->cola_parametros);
    indicaciones_memoria->pid = *auxiliarValores;
    free(auxiliarValores); 
    auxiliarValores = (int *) queue_pop(nueva_peticion->cola_parametros);
    indicaciones_memoria->quantum = *auxiliarValores;
    free(auxiliarValores);
    auxiliarValores = (int *) queue_pop(nueva_peticion->cola_parametros);
    indicaciones_memoria->longitud_direccion_codigo = *auxiliarValores;
    indicaciones_memoria->direccion_codigo = queue_pop(nueva_peticion->cola_parametros);

    free(auxiliarValores);
    return indicaciones_memoria;
} 

t_buffer * serializarBorrarMemoriaP(peticion_memoria * nueva_peticion){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) //PID
    );

    uint32_t * pid = (uint32_t *) queue_pop(nueva_peticion->cola_parametros);
    agregarABufferUint32(buffer, *pid);

    free(pid);
    return buffer;
}

void cargarProcesoEnSistema(uint32_t pid) {
    t_estado_proceso * estado_nuevo = malloc(sizeof(t_estado_proceso));
    estado_nuevo->bloqueadoPorRecurso = false;
    estado_nuevo->elemento_bloqueador = NULL;
    estado_nuevo->estado = NEW;
    estado_nuevo->PID = pid;
    estado_nuevo->recursos_tomados = list_create();
    estado_nuevo->esta_en_ready_plus = false;

    agregarAListaEstados(estado_nuevo);
}