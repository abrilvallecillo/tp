#include "planificacion.h"
#include <utils/conexiones.h>
#include "kernel.h"
#include <commons/temporal.h>
#include <utils/logger_concurrente.h>
#include <utils/procesos.h>
#include <utils/hilos.h>
#include <semaphore.h>
#include <commons/string.h>
#include "configuracion.h"

void *planificarProcesos(void * sin_parametro){
    int conexion_cpu_dispatch = crearConexionCliente(configuracion.IP_CPU, configuracion.PUERTO_CPU_DISPATCH);
    int handshake_cpu_dispatch = enviarHandshake(conexion_cpu_dispatch, KERNEL);
   
    if(handshake_cpu_dispatch == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con la memoria");
        abort();
    }

    //Crear hilo control new ready y hilo de estado a exit
    pasar_procesos_estado_exit = crearHilo(pasarProcesosEstadoAExit, NULL);
    pasar_procesos_new_ready = crearHilo(pasarProcesosNewAReady, NULL);

    pthread_detach(pasar_procesos_estado_exit);
    pthread_detach(pasar_procesos_new_ready);
    //Seleccionar algoritmo de planificación
    while(1) {
        sem_wait(hay_procesos_cola_ready);
        verSiPararPlanificacion();
        pcb * proximo_proceso_a_ejecutar = seleccionarProcesoPorAlgoritmo();
        modificarEnteroSincronizado(pid_actual, proximo_proceso_a_ejecutar->PID);
        avisarYCambiarEstado(proximo_proceso_a_ejecutar->PID, EXEC);
        enviarContexto(proximo_proceso_a_ejecutar, conexion_cpu_dispatch);
        free(proximo_proceso_a_ejecutar);
        do {
            seHizoSyscallNoBloqueante = false; //Para que solo aplique el while si es que se solicito una syscall no bloqueante
            t_paquete * paquete_pcb_desalojado = recibirPaqueteGeneral(conexion_cpu_dispatch);
            if(!paquete_pcb_desalojado) {
                fprintf(stderr, "Error en la conexion con cpu");
                abort();
            }
            operarSegunMotivoDesalojo(paquete_pcb_desalojado, conexion_cpu_dispatch);
        } while(seHizoSyscallNoBloqueante);
        temporal_destroy(cronometro_vrr);
        
        
    }
    
    eliminarSemaforo(hay_procesos_cola_exit);
    eliminarSemaforo(hay_procesos_cola_new);
    eliminarSemaforo(hay_procesos_cola_ready);
    temporal_destroy(cronometro_vrr);
    liberarConexionCliente(conexion_cpu_dispatch);

    return NULL;
}

void * pasarProcesosEstadoAExit(void * sin_parametro) {
    
    while(1) {
        sem_wait(hay_procesos_cola_exit);
        verSiPararPlanificacion();
        pcb_a_finalizar * pcb_fin = sacarDeCola(cola_exit);
        hacerMensajeFinalizacionProceso(pcb_fin->contexto->PID, pcb_fin->motivo_finalizacion);
        eliminarProceso(pcb_fin);
        if(hacerSignalSegunCambioMP()){
            sem_post(grado_multiprogramacion_habilita);
        }
        
    }

    return NULL;
}

void * pasarProcesosNewAReady(void * sin_parametro) {
    while(1) {
        sem_wait(hay_procesos_cola_new);
        sem_wait(grado_multiprogramacion_habilita);
        verSiPararPlanificacion();
        pcb * pcb_siguiente = (pcb *) sacarDeCola(cola_new);
        agregarAColaReadyComunOFinalizar(pcb_siguiente);
    }

    return NULL;
}


pcb * seleccionarProcesoPorAlgoritmo(void) { //Devuelve el proceso seleccionado por el algoritmo 
    switch (algoritmo_elegido)
    {
    case FIFO:
        return obtenerProcesoPorFIFO();        
    case RR:
        return obtenerProcesoPorRR();
    case VRR:
        return obtenerProcesoPorVRR();
    default:
        fprintf(stderr, "Se selecciono un algoritmo invalido");
        abort();
    }
}


pcb * obtenerProcesoPorFIFO(void) {
    pcb * proceso_elegido = (pcb *) sacarDeCola(cola_ready);
    return proceso_elegido;
}

pcb * obtenerProcesoPorRR(void) {
    pcb * proceso_elegido = (pcb *) sacarDeCola(cola_ready);
    return proceso_elegido;
}

pcb * obtenerProcesoPorVRR(void) {
    pcb * proceso_elegido;
    if(colaSincronizadaEstaVacia(cola_ready_vrr)){
        proceso_elegido = (pcb *) sacarDeCola(cola_ready);
        return proceso_elegido;
    }
    //Si no esta vacia
    proceso_elegido = (pcb *) sacarDeCola(cola_ready_vrr);
    return proceso_elegido;
}

void enviarContexto(pcb * proceso_a_enviar, int conexion_cpu_dispatch) {
    t_buffer * buffer_de_pcb = serializarProceso(proceso_a_enviar);
    enviarBufferPorPaquete(buffer_de_pcb, CONTEXTO_A_EJECUTAR, conexion_cpu_dispatch);
    if(algoritmo_elegido == RR || algoritmo_elegido == VRR) {
        modificarEnteroSincronizado(quantum_actual, (uint32_t) proceso_a_enviar->Quantum);
        hilo_timer = crearHilo(contabilizarTiempoEjecucion, NULL);
        pthread_detach(hilo_timer);
    }
    cronometro_vrr = temporal_create();
}


void operarSegunMotivoDesalojo(t_paquete * proceso_desalojado_paq, int conexion_cpu_dispatch) {
    pcb * proceso_recibido; 
    codigos_operacion motivo = proceso_desalojado_paq->codigoOperacion;
    switch (motivo){
        case P_EXIT:
            proceso_recibido = deserializarProceso(proceso_desalojado_paq->buffer);
            temporal_stop(cronometro_vrr);
            if(algoritmo_elegido != FIFO)
                pthread_cancel(hilo_timer);
            verSiPararPlanificacion();
            agregarAColaExit(proceso_recibido, "SUCCESS");
            break;
        case INT_FIN_CONSOLA:
            proceso_recibido = deserializarProceso(proceso_desalojado_paq->buffer);
            temporal_stop(cronometro_vrr);
            if(algoritmo_elegido != FIFO)
                pthread_cancel(hilo_timer);
            verSiPararPlanificacion();
            agregarAColaExit(proceso_recibido, "INTERRUPTED_BY_USER");
            break;
        case INT_FIN_QUANTUM:
            proceso_recibido = deserializarProceso(proceso_desalojado_paq->buffer);
            temporal_stop(cronometro_vrr);
            hacerMensajeFinQuantum(proceso_recibido->PID);
            verSiPararPlanificacion();
            agregarAColaReadyComunOFinalizar(proceso_recibido);
            break;
        case OUT_OF_MEMORY:
            proceso_recibido = deserializarProceso(proceso_desalojado_paq->buffer);
            temporal_stop(cronometro_vrr);
            verSiPararPlanificacion();
            agregarAColaExit(proceso_recibido, "OUT_OF_MEMORY");
            break;
        case P_IO_GEN_SLEEP:
        case P_IO_STDIN_READ:
        case P_IO_STDOUT_WRITE:
        case P_IO_FS_CREATE:
        case P_IO_FS_DELETE:
        case P_IO_FS_TRUNCATE:
        case P_IO_FS_WRITE:
        case P_IO_FS_READ:
            hacerOperacionIO(proceso_desalojado_paq);
            break;
        case P_WAIT:
        case P_SIGNAL:
            pcb_recurso * pcb_wait_o_signal = deserializarProcesoRecurso(proceso_desalojado_paq->buffer);
            haceWaitOSignal(pcb_wait_o_signal, motivo, conexion_cpu_dispatch);
            break;
        default:
            fprintf(stderr, "Error: Motivo de contexto devuelto desconocido");
            break;
    }

    eliminar_paquete(proceso_desalojado_paq);
}



void * contabilizarTiempoEjecucion(void * sin_parametro) {
    usleep(leerEnteroSincronizado(quantum_actual) * 1000);
    sem_post(no_surgio_otra_interrupcion);
    return NULL;
}


void eliminarProceso(pcb_a_finalizar * pcb_fin) {
    peticion_memoria * borrar_memoria = crearPeticionMemoria(P_BORRAR_MEMORIA);
    uint32_t * pid = malloc(sizeof(uint32_t)); 
    *pid = pcb_fin->contexto->PID;
    queue_push(borrar_memoria->cola_parametros, pid);
    agregarACola(cola_memoria, borrar_memoria);
    sem_post(hay_peticiones_memoria);
    liberarRecursosYborrarEstadoSistema(pcb_fin->contexto->PID);
    destruirPcbAFinalizar(pcb_fin);
}

bool hacerSignalSegunCambioMP() {
    int delta_MP = leerEnteroSincronizado(diferencia_cambio_mp); 
    if (delta_MP < 0) {
        incrementarEnteroSincronizado(diferencia_cambio_mp);
    } 
    return delta_MP >= 0;
}

void actualizarQuantum(pcb * pcb_a_actualizar) {
    if(algoritmo_elegido != FIFO)
        pthread_cancel(hilo_timer);
    temporal_stop(cronometro_vrr);
    if(algoritmo_elegido == VRR) { //Solo lo hace si el algoritmo usado es VRR 
        pcb_a_actualizar->Quantum = maxI((int64_t) leerEnteroSincronizado(quantum_actual) - temporal_gettime(cronometro_vrr), 0);
    } else {
        pcb_a_actualizar->Quantum = configuracion.QUANTUM;
    }
}

uint32_t maxI(int a, int b) {
    if(a > b)
        return (uint32_t) a;
    return (uint32_t) b;
}

void haceWaitOSignal(pcb_recurso * pcb, codigos_operacion waitOSignal, int conexion_cpu_dispatch) {
    t_recurso * recurso = buscarRecurso(pcb->nombre_recurso);
    if(recurso) {
        switch(waitOSignal) {
            case P_WAIT:
                aplicarWait(pcb->contexto, recurso, conexion_cpu_dispatch);
                break;
            case P_SIGNAL:
                aplicarSignal(pcb->contexto, recurso, conexion_cpu_dispatch);
                break;
            default:
                fprintf(stderr, "Error, hubo un error al recibir la operacion");
                break;
        }
    } else {
        verSiPararPlanificacion();
        agregarAColaExit(pcb->contexto, "INVALID_RESOURCE");
    }
    free(pcb->nombre_recurso);
    free(pcb);
}

void aplicarWait(pcb * pcb_hace_wait, t_recurso * recurso, int conexion_cpu_dispatch) {
    pthread_mutex_lock(recurso->mutex_cantidad_instancias);
    recurso->cantidad_instancias--;
    actualizarDatosRecursoEnProceso(pcb_hace_wait->PID, recurso->nombre_recurso, 1);
    if(recurso->cantidad_instancias < 0) {
        pthread_mutex_unlock(recurso->mutex_cantidad_instancias);
        actualizarQuantum(pcb_hace_wait);
        verSiPararPlanificacion();
        hacerWaitOFinalizar(recurso, pcb_hace_wait);
    } else {
        pthread_mutex_unlock(recurso->mutex_cantidad_instancias);
        t_buffer * buffer = serializarProceso(pcb_hace_wait);
        enviarBufferProcesoConMotivo(buffer, CONTEXTO_VUELVE_INTERRUPCION, conexion_cpu_dispatch);
        free(pcb_hace_wait);
        seHizoSyscallNoBloqueante = true;
    }
}

void aplicarSignal(pcb * pcb_hace_signal, t_recurso * recurso, int conexion_cpu_dispatch) {
    pthread_mutex_lock(recurso->mutex_cantidad_instancias);
    recurso->cantidad_instancias++;
    pthread_mutex_unlock(recurso->mutex_cantidad_instancias);
    actualizarDatosRecursoEnProceso(pcb_hace_signal->PID, recurso->nombre_recurso, -1);
    if(!colaSincronizadaEstaVacia(recurso->cola_procesos_bloqueados)) {
        
        pcb * proceso_bloqueado = (pcb * ) sacarDeCola(recurso->cola_procesos_bloqueados);
        agregarAColaReadyPorQuantumOFinalizar(proceso_bloqueado);
    } 
    t_buffer * buffer = serializarProceso(pcb_hace_signal);
    enviarBufferProcesoConMotivo(buffer, CONTEXTO_VUELVE_INTERRUPCION, conexion_cpu_dispatch);
    free(pcb_hace_signal);
    seHizoSyscallNoBloqueante = true;
}


void hacerOperacionIO(t_paquete * paquete_io) {
    pcb_cola_interfaz * pcb_a_cola = malloc(sizeof(pcb_cola_interfaz));
    pcb_a_cola->cola_parametros = queue_create();

    char * nombre_interfaz = deserializacionOperacionesIO(pcb_a_cola, paquete_io);
    
    if(!existeInterfazEnSistema(nombre_interfaz) || !interfazPuedeRealizarOperacion(paquete_io->codigoOperacion, nombre_interfaz)) {
        verSiPararPlanificacion();
        agregarAColaExit(pcb_a_cola->contexto, "INVALID_INTERFACE");
        queue_destroy_and_destroy_elements(pcb_a_cola->cola_parametros, free);
        free(pcb_a_cola);
        free(nombre_interfaz);
    } else {
        actualizarQuantum(pcb_a_cola->contexto);
        t_interfaz_kernel * interfaz_elegida = encontrarInterfazEnLista(nombre_interfaz);
        verSiPararPlanificacion();
        bloquearEnInterfazOFinalizar(pcb_a_cola, interfaz_elegida, nombre_interfaz);
    }

}

char * deserializacionOperacionesIO(pcb_cola_interfaz * pcb_a_cola, t_paquete * paquete_io) {
    char * nombre_interfaz;
    pcb_a_cola->operacion = paquete_io->codigoOperacion;
    switch (paquete_io->codigoOperacion)
    {
    case P_IO_GEN_SLEEP:
        pcb_IOGENSLEEP * pcb_gensleep = deserealizarProcesoIOGENSLEEP(paquete_io->buffer);
        cargarPcbColaInterfazIOGENSLEEP(pcb_a_cola, pcb_gensleep);
        nombre_interfaz = string_duplicate(pcb_gensleep->nombre_interfaz);
        free(pcb_gensleep->nombre_interfaz);
        free(pcb_gensleep);
        break;
    
    case P_IO_STDIN_READ:
    case P_IO_STDOUT_WRITE:
        pcb_IOSTD_IN_OUT * pcb_in_out = deserializarProcesoIOSTDINOUT(paquete_io->buffer);
        cargarPcbColaInterfazIOSTDInOut(pcb_a_cola, pcb_in_out);
        nombre_interfaz = string_duplicate(pcb_in_out->nombre_interfaz);
        free(pcb_in_out->nombre_interfaz);
        free(pcb_in_out);
        break;
    case P_IO_FS_CREATE:
    case P_IO_FS_DELETE:
        pcb_FSCREADEL * pcb_CREADEL = deserializarProcesoFSCREADEL(paquete_io->buffer);
        cargarPcbColaInterfazFSCREADEL(pcb_a_cola, pcb_CREADEL);
        nombre_interfaz = string_duplicate(pcb_CREADEL->nombre_interfaz);
        free(pcb_CREADEL->nombre_interfaz);
        free(pcb_CREADEL);
        break;
    case P_IO_FS_TRUNCATE:
        pcb_FSTRUN * pcb_TRUN = deserializarProcesoFSTRUN(paquete_io->buffer);
        cargarPcbColaInterfazFSTRUN(pcb_a_cola, pcb_TRUN);
        nombre_interfaz = string_duplicate(pcb_TRUN->nombre_interfaz);
        free(pcb_TRUN->nombre_interfaz);
        free(pcb_TRUN);
        break;
    case P_IO_FS_READ:
    case P_IO_FS_WRITE:
        pcb_FSWR * pcb_WR = deserializarProcesoFSWR(paquete_io->buffer);
        cargarPcbColaInterfazFSWR(pcb_a_cola, pcb_WR);
        nombre_interfaz = string_duplicate(pcb_WR->nombre_interfaz);
        free(pcb_WR->nombre_interfaz);
        free(pcb_WR);
        break;
    default:
        fprintf(stderr, "Error, operacion de entrada salida desconocida");
        break;
    }

    return nombre_interfaz;
}

void cargarPcbColaInterfazIOGENSLEEP(pcb_cola_interfaz * pcb_a_cola, pcb_IOGENSLEEP * pcb_gensleep) {
    pcb_a_cola->contexto = pcb_gensleep->contexto;
    uint32_t* unidades_de_trabajo = malloc(sizeof(uint32_t));
    *unidades_de_trabajo = pcb_gensleep->unidades_de_trabajo;
    queue_push(pcb_a_cola->cola_parametros, unidades_de_trabajo);
}

void cargarPcbColaInterfazIOSTDInOut(pcb_cola_interfaz * pcb_a_cola, pcb_IOSTD_IN_OUT * pcb_in_out) {
    pcb_a_cola->contexto = pcb_in_out->contexto;
    uint32_t* cantidadDirecciones = malloc(sizeof(uint32_t));
    *cantidadDirecciones = pcb_in_out->cantidad_direccionesFisicas;
    queue_push(pcb_a_cola->cola_parametros, cantidadDirecciones);
    queue_push(pcb_a_cola->cola_parametros, pcb_in_out->direccionesFisicas);
}

void cargarPcbColaInterfazFSCREADEL(pcb_cola_interfaz * pcb_a_cola, pcb_FSCREADEL * pcb_CREADEL){
    pcb_a_cola->contexto = pcb_CREADEL->contexto;
    queue_push(pcb_a_cola->cola_parametros, pcb_CREADEL->nombre_archivo);
}

void cargarPcbColaInterfazFSTRUN(pcb_cola_interfaz * pcb_a_cola, pcb_FSTRUN * pcb_TRUN) {
    pcb_a_cola->contexto = pcb_TRUN->contexto;
    queue_push(pcb_a_cola->cola_parametros, pcb_TRUN->nombre_archivo);
    uint32_t* tamanio_archivo = malloc(sizeof(uint32_t));
    *tamanio_archivo = pcb_TRUN->tamanio_nuevo_archivo;
    queue_push(pcb_a_cola->cola_parametros, tamanio_archivo);
}

void cargarPcbColaInterfazFSWR(pcb_cola_interfaz * pcb_a_cola, pcb_FSWR * pcb_WR) {
    pcb_a_cola->contexto = pcb_WR->contexto;
    queue_push(pcb_a_cola->cola_parametros, pcb_WR->nombre_archivo);
    uint32_t* puntero_archivo = malloc(sizeof(uint32_t));
    *puntero_archivo = pcb_WR->punteroArchivo;
    queue_push(pcb_a_cola->cola_parametros, puntero_archivo);
    uint32_t* cantidadDirecciones = malloc(sizeof(uint32_t));
    *cantidadDirecciones = pcb_WR->cantidad_direccionesFisicas;
    queue_push(pcb_a_cola->cola_parametros, cantidadDirecciones);
    queue_push(pcb_a_cola->cola_parametros, pcb_WR->direccionesFisicas);
}

void hacerMensajeFinQuantum(uint32_t pid) {
    char * mensaje_fin_q = string_from_format("PID: %d - Desalojado por fin de Quantum", pid);
    logInfoSincronizado(mensaje_fin_q);
    free(mensaje_fin_q);
}

void hacerMensajeFinalizacionProceso(uint32_t pid, char *motivo_finalizacion){
    char * mensaje_finalizacion = string_from_format("Finaliza el proceso %d - Motivo: %s", pid, motivo_finalizacion);
    logInfoSincronizado(mensaje_finalizacion);
    free(mensaje_finalizacion);
}