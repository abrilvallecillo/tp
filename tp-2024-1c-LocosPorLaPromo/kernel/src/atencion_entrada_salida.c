#include "atencion_entrada_salida.h"
#include <utils/conexiones.h>
#include <semaphore.h>
#include <utils/procesos.h>
#include <utils/logger_concurrente.h>
#include "kernel.h"
#include <utils/interfaces.h>
#include <commons/string.h>

t_interfaz_kernel * recibirTipoInterfaz(int conexion_cliente_io);
t_interfaz_kernel * deserializarInformacionInterfaz(t_buffer * buffer);
void enviarOperacionAInterfaz(pcb_cola_interfaz * pcb_a_trabajar, int conexion_cliente_io);
int recibirOperacionAInterfaz(int conexion_cliente_io);
void eliminarInterfaz(t_interfaz_kernel * interfaz_actual);
void mandarAFinalizarLosProcesos(void * elemento);
void matarProcesoInterfaz(pcb_cola_interfaz *proceso_a_matar);

void * atenderIO(void * socket_cliente_io) {
    int* conexion_cliente_io = (int *) socket_cliente_io;
    int resultado = recibirHandshake(*conexion_cliente_io);
    if(resultado == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con la interfaz");
        abort();
    }

    t_interfaz_kernel * interfaz_actual = recibirTipoInterfaz(*conexion_cliente_io); //Inicializara ademas el semaforo de hay_procesos_en_interfaz
    t_mutex_eliminacion_interfaz * nuevo_mutex = malloc(sizeof(t_mutex_eliminacion_interfaz));
    nuevo_mutex->nombre_interfaz = string_duplicate(interfaz_actual->nombre_interfaz);
    nuevo_mutex->mutex = malloc(sizeof(pthread_mutex_t));
    crearMutex(nuevo_mutex->mutex);
    agregarMutexInterfazAlSistema(nuevo_mutex);
    agregarInterfazALista(interfaz_actual);
    while(1) {
        sem_wait(interfaz_actual->hay_procesos_en_interfaz);
        pcb_cola_interfaz * pcb_a_trabajar = sacarDeCola(interfaz_actual->cola_bloqueados);
        enviarOperacionAInterfaz(pcb_a_trabajar, *conexion_cliente_io);
        int salio_bien_operacion = recibirOperacionAInterfaz(*conexion_cliente_io);
        verSiPararPlanificacion();
        if(salio_bien_operacion == -1) {
            matarProcesoInterfaz(pcb_a_trabajar);
            break; 
        }
        agregarAColaReadyPorQuantumOFinalizar(pcb_a_trabajar->contexto);
        
        queue_destroy(pcb_a_trabajar->cola_parametros);
        free(pcb_a_trabajar);
    }
    eliminarInterfaz(interfaz_actual);
    liberarConexionCliente(*conexion_cliente_io);
    free(conexion_cliente_io);
    return NULL;
}

t_interfaz_kernel * recibirTipoInterfaz(int conexion_cliente_io) {
    t_paquete * paquete_informacion_interfaz = recibirPaqueteGeneral(conexion_cliente_io);
    t_interfaz_kernel * interfaz_obtenida = deserializarInformacionInterfaz(paquete_informacion_interfaz->buffer);
    eliminar_paquete(paquete_informacion_interfaz);
    return interfaz_obtenida;
}

t_interfaz_kernel * deserializarInformacionInterfaz(t_buffer * buffer) {
    t_interfaz_kernel * interfaz = malloc(sizeof(t_interfaz_kernel));
    
    uint32_t longitud_nombre = leerDeBufferUint32(buffer);
    interfaz->nombre_interfaz = leerDeBufferString(buffer, longitud_nombre);
    interfaz->tipo_de_interfaz = leerDeBufferUint8(buffer);
    interfaz->cola_bloqueados = crearCola();
    interfaz->hay_procesos_en_interfaz = crearSemaforo(0);

    return interfaz;
}

void enviarOperacionAInterfaz(pcb_cola_interfaz * pcb_a_trabajar, int conexion_cliente_io) { //Aca ya suponemos que se ha elegido correctamente el tipo de operacion que corresponde a la interfaz
    t_buffer * buffer;
    switch(pcb_a_trabajar->operacion) {
        case P_IO_GEN_SLEEP:
            buffer = serializarOperacionIOGENSLEEP(pcb_a_trabajar);
            break;
        case P_IO_STDOUT_WRITE:
        case P_IO_STDIN_READ:
            buffer = serializarOperacionSTDInOut(pcb_a_trabajar);
            break;
        case P_IO_FS_CREATE:
        case P_IO_FS_DELETE:
            buffer = serializarOperacionFSCREADEL(pcb_a_trabajar);
            break;
        case P_IO_FS_TRUNCATE:
            buffer = serializarOperacionFSTRUN(pcb_a_trabajar);
            break;
        case P_IO_FS_WRITE:
        case P_IO_FS_READ:
            buffer = serializarOperacionFSWR(pcb_a_trabajar);
            break;
        default:
            fprintf(stderr, "Error: se ha enviado un codigo de operacion no reconocido");
            return;
    }

    enviarBufferProcesoConMotivo(buffer, pcb_a_trabajar->operacion, conexion_cliente_io);
}

int recibirOperacionAInterfaz(int conexion_cliente_io) {
    t_paquete * paq_salio_bien_operacion= recibirPaqueteGeneral(conexion_cliente_io);
    if(paq_salio_bien_operacion == NULL) {
        return -1;   
    }
    eliminar_paquete(paq_salio_bien_operacion);
    return 0;
}

void eliminarInterfaz(t_interfaz_kernel * interfaz_actual) {
    eliminarInterfazDeSistema(interfaz_actual);
    t_mutex_eliminacion_interfaz * elemento = buscarMutexEliminacionLista(interfaz_actual->nombre_interfaz);
    pthread_mutex_lock(elemento->mutex);
    free(interfaz_actual->nombre_interfaz);
    eliminarSemaforo(interfaz_actual->hay_procesos_en_interfaz);
    if(!colaSincronizadaEstaVacia(interfaz_actual->cola_bloqueados))
        eliminarColaSincronizadaYElementos(interfaz_actual->cola_bloqueados, mandarAFinalizarLosProcesos);
    else
        eliminarColaSincronizada(interfaz_actual->cola_bloqueados);
    free(interfaz_actual);
    pthread_mutex_unlock(elemento->mutex);
}

void mandarAFinalizarLosProcesos(void * elemento) {
    pcb_cola_interfaz * proceso_a_matar = (pcb_cola_interfaz *) elemento;
    matarProcesoInterfaz(proceso_a_matar);
}

void matarProcesoInterfaz(pcb_cola_interfaz *proceso_a_matar) {
    agregarAColaExit(proceso_a_matar->contexto, "INVALID_INTERFACE");
    queue_destroy_and_destroy_elements(proceso_a_matar->cola_parametros, free);
    free(proceso_a_matar);
}