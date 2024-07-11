#include "kernel.h"
#include <string.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <utils/hilos.h>
#include <utils/logger_concurrente.h>
#include "configuracion.h"

cola_sincronizada * cola_new;
cola_sincronizada * cola_ready;
cola_sincronizada * cola_ready_vrr;
cola_sincronizada * cola_exit;
cola_sincronizada * cola_memoria;
algoritmo algoritmo_elegido;
t_list * lista_interfaces;
sem_t * solicitaron_finalizar_proceso;
sem_t * no_surgio_otra_interrupcion;
sem_t * hay_peticion_en_cola;
pthread_t  hilo_timer;
pthread_mutex_t * mutex_lista_interfaces;
char * nombre_interfaz_a_comparar;
entero_sincronizado * grado_multiprogramacion;
int pid_nuevo_proceso;
sem_t * hay_peticiones_memoria;
t_list * lista_recursos;
entero_sincronizado * diferencia_cambio_mp;
t_list * lista_estados;
sem_t * hay_que_parar_planificacion;
entero_sincronizado * pid_fin_usuario;
lista_sincronizada * lista_mutex_eliminacion_interfaz;
entero_sincronizado * pid_actual;
sem_t * grado_multiprogramacion_habilita;
sem_t * hay_procesos_cola_exit;
sem_t * hay_procesos_cola_new;
sem_t * hay_procesos_cola_ready;
entero_sincronizado * quantum_actual;

bool interfazTieneNombre(void * interfaz);
bool compararNombre(void * recurso);
bool encontrarEstadoPid(void * elemento);
pcb * buscarYExtraerDeCola(cola_sincronizada * cola_sinc, uint32_t pid);
bool obtenerBloqueadorPorPID(uint32_t pid);
char * obtenerNombreBloqueadorPorPID(uint32_t pid);
void setearTipoBloqueadorYNombreEnSistema(uint32_t pid, bool es_recurso, char * nombre_bloqueador);
bool compararNombreRecursoSistema(void * recurso_sistema);
t_estado_proceso * encontrarEstadoPorPID(uint32_t pid);
bool encontrarPcbPID(void * proces);
pcb * buscarPCBAEliminar(estado_procesos estado_actual, uint32_t pid);
void liberarInstanciasRecurso(void * recurso_tom);
void listarPIDsEnReadyComun();
void listarPIDsEnReadyPlus();
void mostrarPIDsYEstados(void * elemento_estado);
void * transformarEstadoProcesoAString(void * elemento);
bool encontrarMutexEliminacionPorNombre(void * elemento);
void indicarSiEstaEnReadyComunONo(uint32_t pid, bool esReadyPlus);

algoritmo algoritmoPorString(char* algoritmo) {
    if(strcmp(algoritmo, "FIFO") == 0) 
        return FIFO;
    else if(strcmp(algoritmo, "RR") == 0)
        return RR;
    else
        return VRR;
}

cola_sincronizada * crearCola() {
    cola_sincronizada * cola_sincro = malloc(sizeof(cola_sincronizada));
    cola_sincro->cola = queue_create();
    if(!cola_sincro->cola) {
        fprintf(stderr, "Error al crear la cola");
        abort();
    }
    cola_sincro->mutex_cola = malloc(sizeof(pthread_mutex_t));
    crearMutex(cola_sincro->mutex_cola);

    return cola_sincro;
}

void agregarACola(cola_sincronizada * cola_elegida, void * valor) {
    pthread_mutex_lock(cola_elegida->mutex_cola);
    queue_push(cola_elegida->cola, valor);
    pthread_mutex_unlock(cola_elegida->mutex_cola);
}
void * sacarDeCola(cola_sincronizada * cola_elegida) {
    pthread_mutex_lock(cola_elegida->mutex_cola);
    void * valor = queue_pop(cola_elegida->cola);
    pthread_mutex_unlock(cola_elegida->mutex_cola);
    return valor;
}

bool colaSincronizadaEstaVacia(cola_sincronizada * cola_elegida) {
    pthread_mutex_lock(cola_elegida->mutex_cola);
    bool resultado = queue_is_empty(cola_elegida->cola);
    pthread_mutex_unlock(cola_elegida->mutex_cola);
    return resultado;
}

void eliminarColaSincronizadaYElementos(cola_sincronizada * cola_elegida, void (*funcion_eliminadora) (void *)) {
    pthread_mutex_lock(cola_elegida->mutex_cola);
    queue_destroy_and_destroy_elements(cola_elegida->cola, funcion_eliminadora);
    pthread_mutex_unlock(cola_elegida->mutex_cola);
    pthread_mutex_destroy(cola_elegida->mutex_cola);
    free(cola_elegida->mutex_cola);
    free(cola_elegida);
}

void eliminarColaSincronizada(cola_sincronizada * cola_elegida) {
    queue_destroy(cola_elegida->cola);
    pthread_mutex_destroy(cola_elegida->mutex_cola);
    free(cola_elegida->mutex_cola);
    free(cola_elegida);
}

void avisarCambioEstado(uint32_t pid, const char * estado_anterior, const char * estado_actual) {
    char * aviso_cambio_estado = string_from_format("PID: %d - Estado Anterior: %s - Estado Actual: %s ", pid, estado_anterior, estado_actual);
    logInfoSincronizado(aviso_cambio_estado);
    free(aviso_cambio_estado);
}

void avisarBloqueo(uint32_t pid, const char * nombre_recurso_o_interfaz) {
    char * avisarBloqueo = string_from_format("PID: %d - Bloqueado por: %s", pid, nombre_recurso_o_interfaz);
    logInfoSincronizado(avisarBloqueo);
    free(avisarBloqueo);
}

bool existeInterfazEnSistema(char * nombre_interfaz) {
    pthread_mutex_lock(mutex_lista_interfaces);
    nombre_interfaz_a_comparar = string_duplicate(nombre_interfaz);
    bool resultado = list_any_satisfy(lista_interfaces, interfazTieneNombre);
    free(nombre_interfaz_a_comparar);
    pthread_mutex_unlock(mutex_lista_interfaces);
    return resultado;
}

bool interfazTieneNombre(void * interfaz) {
    t_interfaz_kernel * interfaz_elegida = (t_interfaz_kernel *) interfaz;
    return !strcmp(nombre_interfaz_a_comparar, interfaz_elegida->nombre_interfaz);
}
bool interfazPuedeRealizarOperacion(codigos_operacion codigo, char * nombre_interfaz) {
    t_interfaz_kernel * interfaz_encontrada = encontrarInterfazEnLista(nombre_interfaz);
    
    if(interfaz_encontrada == NULL)
        return false;
    
    switch(codigo){
        case P_IO_GEN_SLEEP:
            return interfaz_encontrada->tipo_de_interfaz == GENERICA;
        case P_IO_STDIN_READ:
            return interfaz_encontrada->tipo_de_interfaz == STDIN;
        case P_IO_STDOUT_WRITE:
            return interfaz_encontrada->tipo_de_interfaz == STDOUT;
        case P_IO_FS_CREATE:
        case P_IO_FS_DELETE:
        case P_IO_FS_READ:
        case P_IO_FS_TRUNCATE:
        case P_IO_FS_WRITE:
            return interfaz_encontrada->tipo_de_interfaz == DIALFS;
        default:
            fprintf(stderr, "Error: operacion desconocida!");
            return false;
    }
}

t_interfaz_kernel * encontrarInterfazEnLista(char * nombre_interfaz) {
    pthread_mutex_lock(mutex_lista_interfaces);
    nombre_interfaz_a_comparar = string_duplicate(nombre_interfaz);
    t_interfaz_kernel * interfaz_encontrada = (t_interfaz_kernel *) list_find(lista_interfaces, interfazTieneNombre);
    free(nombre_interfaz_a_comparar);
    pthread_mutex_unlock(mutex_lista_interfaces);
    return interfaz_encontrada;
}

void agregarInterfazALista(t_interfaz_kernel * interfaz_actual) {
    pthread_mutex_lock(mutex_lista_interfaces);
    list_add(lista_interfaces, interfaz_actual);
    pthread_mutex_unlock(mutex_lista_interfaces);
}



void agregarAColaReadySegunAlgoritmoQuantum(pcb *proceso_bloqueado) {
    uint32_t pid = proceso_bloqueado->PID;
    avisarYCambiarEstado(pid, READY);
    if(proceso_bloqueado->Quantum > 0 && algoritmo_elegido == VRR) {
        indicarSiEstaEnReadyComunONo(pid, true);
        agregarACola(cola_ready_vrr, proceso_bloqueado);
        listarPIDsEnReadyPlus();
        sem_post(hay_procesos_cola_ready);
    } else {
        indicarSiEstaEnReadyComunONo(pid, false);
        agregarAColaReadyComun(proceso_bloqueado);
    }
}

void agregarAColaReadyComun(pcb *proceso) {
    proceso->Quantum = configuracion.QUANTUM;
    agregarACola(cola_ready, proceso);
    listarPIDsEnReadyComun();
    sem_post(hay_procesos_cola_ready);
}

char * nombre_a_comparar;
pthread_mutex_t * mutex_nombre_recurso;


t_recurso * buscarRecurso(char * nombre_recurso) {
    pthread_mutex_lock(mutex_nombre_recurso);
    nombre_a_comparar = string_duplicate(nombre_recurso);
    t_recurso * recurso = (t_recurso *) list_find(lista_recursos, compararNombre);
    free(nombre_a_comparar);
    pthread_mutex_unlock(mutex_nombre_recurso);
    return recurso;
}

bool compararNombre(void * recurso) {
    t_recurso * recurso_elegido = (t_recurso *) recurso;
    return !strcmp(recurso_elegido->nombre_recurso, nombre_a_comparar);
}

void cargarRecursos() {
    while(!string_array_is_empty(configuracion.RECURSOS)){
        t_recurso * recurso = malloc(sizeof(t_recurso));
        recurso->nombre_recurso = string_array_pop(configuracion.RECURSOS);
        char * instancia_recurso = string_array_pop(configuracion.INSTANCIAS_RECURSOS);
        recurso->cantidad_instancias = atoi(instancia_recurso);
        recurso->mutex_cantidad_instancias = malloc(sizeof(pthread_mutex_t));
        crearMutex(recurso->mutex_cantidad_instancias);
        free(instancia_recurso);
        recurso->cola_procesos_bloqueados = crearCola();
        list_add(lista_recursos, recurso);
    }
}

uint32_t pid_comparar_estado;
pthread_mutex_t  * mutex_pid_comparar;

int obtenerEstadoPorPID(uint32_t pid) {
    t_estado_proceso * estado_pedido = encontrarEstadoPorPID(pid);
    if(estado_pedido == NULL)
        return -1;
    return estado_pedido->estado; 
}

bool encontrarEstadoPid(void * elemento) {
    t_estado_proceso * estado_pedido = (t_estado_proceso *) elemento;
    return estado_pedido->PID == pid_comparar_estado;
}

void cambiarEstadoLista(uint32_t pid, estado_procesos estado_nuevo) {
    t_estado_proceso * estado_pedido = encontrarEstadoPorPID(pid);
    estado_pedido->estado = estado_nuevo;
}

char * obtenerEstadoString(estado_procesos estado){
    if(estado == NEW)
        return "NEW";
    else if(estado == READY) 
        return "READY";
    else if(estado == BLOCKED) 
        return "BLOCKED";
    else if(estado == EXEC)
        return "EXEC";
    else
        return "EXIT";
}

void avisarYCambiarEstado(uint32_t pid, estado_procesos estado_actual) {
    char * estado_anterior = obtenerEstadoString(obtenerEstadoPorPID(pid));
    cambiarEstadoLista(pid, estado_actual);
    avisarCambioEstado(pid, estado_anterior, obtenerEstadoString(estado_actual));
}

pcb_a_finalizar * crearPcbAFinalizar(pcb * proceso, char * motivo_fin) {
    pcb_a_finalizar * pcb_fin = malloc(sizeof(pcb_a_finalizar));
    pcb_fin->contexto = proceso;
    pcb_fin->motivo_finalizacion = string_duplicate(motivo_fin);
    return pcb_fin;
}
void destruirPcbAFinalizar(pcb_a_finalizar * pcb_fin) {
    free(pcb_fin->contexto);
    free(pcb_fin->motivo_finalizacion);
    free(pcb_fin);
}

void agregarAColaExit(pcb * proceso, char * motivo_fin){
    uint32_t pid = proceso->PID;
    pcb_a_finalizar * pcb_fin = crearPcbAFinalizar(proceso, motivo_fin);
    agregarACola(cola_exit, pcb_fin);
    avisarYCambiarEstado(pid, EXIT);
    sem_post(hay_procesos_cola_exit);
}

peticion_memoria * crearPeticionMemoria(codigos_operacion operacion) {
    peticion_memoria * peticion = malloc(sizeof(peticion_memoria));
    peticion->operacion = operacion;
    peticion->cola_parametros = queue_create();
    return peticion;
}

void verSiPararPlanificacion() {
    sem_wait(hay_que_parar_planificacion);
    sem_post(hay_que_parar_planificacion);
}

void agregarAColaReadyComunOFinalizar(pcb * proceso) {
    uint32_t pid = proceso->PID;
    bool resultado = pid == (uint32_t) leerEnteroSincronizado(pid_fin_usuario);
    if(resultado) {
        agregarAColaExit(proceso, "INTERRUPTED_BY_USER");
    } else {
        avisarYCambiarEstado(pid, READY);
        indicarSiEstaEnReadyComunONo(pid, false);
        agregarAColaReadyComun(proceso);
    }
}

void agregarAColaReadyPorQuantumOFinalizar(pcb * proceso_bloqueado) {
    if(proceso_bloqueado->PID == (uint32_t) leerEnteroSincronizado(pid_fin_usuario)) {
        agregarAColaExit(proceso_bloqueado, "INTERRUPTED_BY_USER");
    } else {
        agregarAColaReadySegunAlgoritmoQuantum(proceso_bloqueado);
    }
}

void bloquearEnInterfazOFinalizar(pcb_cola_interfaz * pcb_a_cola, t_interfaz_kernel * interfaz_elegida, char * nombre_interfaz) {
    t_mutex_eliminacion_interfaz * elemento = buscarMutexEliminacionLista(nombre_interfaz);
    pthread_mutex_lock(elemento->mutex);
    uint32_t pid = pcb_a_cola->contexto->PID;
    if(pid == (uint32_t) leerEnteroSincronizado(pid_fin_usuario)) {
        agregarAColaExit(pcb_a_cola->contexto, "INTERRUPTED_BY_USER");
        queue_destroy_and_destroy_elements(pcb_a_cola->cola_parametros, free);
        free(pcb_a_cola);
        pthread_mutex_unlock(elemento->mutex);
    } else if(interfaz_elegida == NULL) {
        agregarAColaExit(pcb_a_cola->contexto, "INVALID_INTERFACE");
        queue_destroy_and_destroy_elements(pcb_a_cola->cola_parametros, free);
        free(pcb_a_cola);
        pthread_mutex_unlock(elemento->mutex);
    } else {
        avisarYCambiarEstado(pid, BLOCKED);
        avisarBloqueo(pid, interfaz_elegida->nombre_interfaz);
        setearTipoBloqueadorYNombreEnSistema(pid, false, interfaz_elegida->nombre_interfaz);
        agregarACola(interfaz_elegida->cola_bloqueados, pcb_a_cola);
        sem_post(interfaz_elegida->hay_procesos_en_interfaz);
        pthread_mutex_unlock(elemento->mutex);
    }
    free(nombre_interfaz);
}

void hacerWaitOFinalizar(t_recurso * recurso, pcb * pcb_hace_wait) {
    uint32_t pid = pcb_hace_wait->PID;
    if(pid == (uint32_t) leerEnteroSincronizado(pid_fin_usuario)) {
        agregarAColaExit(pcb_hace_wait, "INTERRUPTED_BY_USER");
    } else {
        avisarYCambiarEstado(pid, BLOCKED);
        avisarBloqueo(pcb_hace_wait->PID, recurso->nombre_recurso);
        setearTipoBloqueadorYNombreEnSistema(pid, true, recurso->nombre_recurso);
        agregarACola(recurso->cola_procesos_bloqueados, pcb_hace_wait);
    }
}

void buscarYTratarDeEliminarEn(estado_procesos estado_actual, uint32_t pid){
    pcb * pcb_a_matar = buscarPCBAEliminar(estado_actual, pid);
    if(pcb_a_matar) {
        agregarAColaExit(pcb_a_matar, "INTERRUPTED_BY_USER");
    }
}

pcb * buscarPCBAEliminar(estado_procesos estado_actual, uint32_t pid) {
    pcb * pcb_encontrado;
    switch (estado_actual) {
        case NEW:
            pcb_encontrado = buscarYExtraerDeCola(cola_new, pid);
            break;
        case READY:
            pcb_encontrado = buscarYExtraerDeCola(cola_ready, pid);
            if(pcb_encontrado == NULL){
                pcb_encontrado = buscarYExtraerDeCola(cola_ready_vrr, pid);
            }
            break;
        case BLOCKED:
            bool esRecurso = obtenerBloqueadorPorPID(pid);
            char * nombre_bloq = obtenerNombreBloqueadorPorPID(pid);
            if(esRecurso) {
                t_recurso * recurso = buscarRecurso(nombre_bloq);
                pcb_encontrado = buscarYExtraerDeCola(recurso->cola_procesos_bloqueados, pid);
            } else {
                t_interfaz_kernel * interfaz = encontrarInterfazEnLista(nombre_bloq);
                if(interfaz != NULL) {
                    pcb_encontrado = buscarYExtraerDeCola(interfaz->cola_bloqueados, pid);
                }
            }
            break;
        default:
            fprintf(stderr, "Error, estado desconocido");
            break;
    }

    return pcb_encontrado;
}

uint32_t pid_cola;

pcb * buscarYExtraerDeCola(cola_sincronizada * cola_sinc, uint32_t pid) {
    pthread_mutex_lock(cola_sinc->mutex_cola);
    pid_cola = pid;
    pcb * pcb_elegido = (pcb *) list_find(cola_sinc->cola->elements, encontrarPcbPID);
    list_remove_element(cola_sinc->cola->elements, pcb_elegido);
    pthread_mutex_unlock(cola_sinc->mutex_cola);
    return pcb_elegido;
}

bool encontrarPcbPID(void * proces) {
    pcb * proceso = (pcb *) proces;
    return proceso->PID == pid_cola;
}

bool obtenerBloqueadorPorPID(uint32_t pid) {
    t_estado_proceso * estado_pedido = encontrarEstadoPorPID(pid);
    return estado_pedido->bloqueadoPorRecurso; 
}

char * obtenerNombreBloqueadorPorPID(uint32_t pid) {
    t_estado_proceso * estado_pedido = encontrarEstadoPorPID(pid);
    return estado_pedido->elemento_bloqueador; 
}

void setearTipoBloqueadorYNombreEnSistema(uint32_t pid, bool es_recurso, char * nombre_bloqueador) {
    t_estado_proceso * estado_a_actualizar = encontrarEstadoPorPID(pid);
    estado_a_actualizar->bloqueadoPorRecurso = es_recurso;
    if (estado_a_actualizar->elemento_bloqueador)
        free(estado_a_actualizar->elemento_bloqueador);
    estado_a_actualizar->elemento_bloqueador = string_duplicate(nombre_bloqueador);
}

char * nombre_recurso_sist_comparar;

void actualizarDatosRecursoEnProceso(uint32_t pid, char * nombre_recurso, int instancias_a_sumar) {
    t_estado_proceso * estado_a_actualizar = encontrarEstadoPorPID(pid);
    nombre_recurso_sist_comparar = nombre_recurso;
    recurso_tomado_proceso * recurso_a_modificar = (recurso_tomado_proceso *)  list_find(estado_a_actualizar->recursos_tomados, compararNombreRecursoSistema);
    if(recurso_a_modificar == NULL) {
        recurso_a_modificar = malloc(sizeof(recurso_tomado_proceso));
        recurso_a_modificar->nombre_recurso = string_duplicate(nombre_recurso);
        recurso_a_modificar->instancias_tomadas = 0;
        list_add(estado_a_actualizar->recursos_tomados, recurso_a_modificar);
    }
    if(instancias_a_sumar > 0 || recurso_a_modificar->instancias_tomadas != 0)
        recurso_a_modificar->instancias_tomadas += instancias_a_sumar;
}

bool compararNombreRecursoSistema(void * recurso_sistema) {
    recurso_tomado_proceso * recurso = (recurso_tomado_proceso *) recurso_sistema;
    return !strcmp(nombre_recurso_sist_comparar, recurso->nombre_recurso); 
}

void liberarRecursosYborrarEstadoSistema(uint32_t pid) {
    pthread_mutex_lock(mutex_pid_comparar);
    pid_comparar_estado = pid;
    t_estado_proceso * estado_a_borrar = (t_estado_proceso *) list_find(lista_estados, encontrarEstadoPid);
    list_remove_element(lista_estados, estado_a_borrar);
    pthread_mutex_unlock(mutex_pid_comparar);
    if(list_size(estado_a_borrar->recursos_tomados) > 0) 
        list_destroy_and_destroy_elements(estado_a_borrar->recursos_tomados, liberarInstanciasRecurso);
    else 
        list_destroy(estado_a_borrar->recursos_tomados);
    free(estado_a_borrar->elemento_bloqueador);
    free(estado_a_borrar);
}

void liberarInstanciasRecurso(void * recurso_tom) {
    recurso_tomado_proceso * recurso_a_liberar = (recurso_tomado_proceso *) recurso_tom;
    t_recurso * recurso = buscarRecurso(recurso_a_liberar->nombre_recurso);
    while(recurso_a_liberar->instancias_tomadas) {
        pthread_mutex_lock(recurso->mutex_cantidad_instancias);
        recurso->cantidad_instancias++;
        pthread_mutex_unlock(recurso->mutex_cantidad_instancias);
        recurso_a_liberar->instancias_tomadas--;
        if(!colaSincronizadaEstaVacia(recurso->cola_procesos_bloqueados)) {
            pcb * proceso_bloqueado = (pcb * ) sacarDeCola(recurso->cola_procesos_bloqueados);
            agregarAColaReadyPorQuantumOFinalizar(proceso_bloqueado);
        }
    }
    free(recurso_a_liberar->nombre_recurso);
    free(recurso_a_liberar);
}

t_estado_proceso * encontrarEstadoPorPID(uint32_t pid) {
    pthread_mutex_lock(mutex_pid_comparar);
    pid_comparar_estado = pid;
    t_estado_proceso * estado = (t_estado_proceso *) list_find(lista_estados, encontrarEstadoPid);
    pthread_mutex_unlock(mutex_pid_comparar);
    return estado;
}

void listarPIDsyEstadosActuales() {
    pthread_mutex_lock(mutex_pid_comparar);
    list_iterate(lista_estados, mostrarPIDsYEstados);
    pthread_mutex_unlock(mutex_pid_comparar);
}

void mostrarPIDsYEstados(void * elemento_estado) {
    t_estado_proceso * estado_proces = (t_estado_proceso *)elemento_estado;
    printf("\nPID: %d -Estado: %s", estado_proces->PID, obtenerEstadoString(estado_proces->estado));
}

void listarPIDsEnReadyComun() {
    pthread_mutex_lock(cola_ready->mutex_cola);
    t_list * lista_proces_ready = cola_ready->cola->elements;
    t_list * lista_string_pid_ready = list_map(lista_proces_ready, transformarEstadoProcesoAString);
    char * mensaje_en_partes = string_new();
    string_append(&mensaje_en_partes, "Cola Ready: [ ");
    while(!list_is_empty(lista_string_pid_ready)) {
        char * pid = list_remove(lista_string_pid_ready, 0);
        string_append(&mensaje_en_partes, pid);
        free(pid);
    }
    string_append(&mensaje_en_partes, " ]");
    logInfoSincronizado(mensaje_en_partes);
    list_destroy(lista_string_pid_ready);
    free(mensaje_en_partes);
    pthread_mutex_unlock(cola_ready->mutex_cola);
}


void listarPIDsEnReadyPlus() {
    pthread_mutex_lock(cola_ready_vrr->mutex_cola);
    t_list * lista_proces_ready = cola_ready_vrr->cola->elements;
    t_list * lista_string_pid_ready = list_map(lista_proces_ready, transformarEstadoProcesoAString);
    char * mensaje_en_partes = string_new();
    string_append(&mensaje_en_partes, "Cola Ready Prioridad: [ ");
    while(!list_is_empty(lista_string_pid_ready)) {
        char * pid = list_remove(lista_string_pid_ready, 0);
        string_append(&mensaje_en_partes, pid);
        free(pid);
    }
    string_append(&mensaje_en_partes, " ]");
    logInfoSincronizado(mensaje_en_partes);
    list_destroy(lista_string_pid_ready);
    free(mensaje_en_partes);
    pthread_mutex_unlock(cola_ready_vrr->mutex_cola);
}

void * transformarEstadoProcesoAString(void * elemento) {
    pcb * proceso = (pcb *) elemento;
    return string_from_format("%d, ", proceso->PID);
}

void agregarAListaEstados(t_estado_proceso *estado_nuevo) {
    pthread_mutex_lock(mutex_pid_comparar);
    list_add(lista_estados, estado_nuevo);
    pthread_mutex_unlock(mutex_pid_comparar);
}

void eliminarInterfazDeSistema(t_interfaz_kernel * interfaz_actual) {
    pthread_mutex_lock(mutex_lista_interfaces);
    list_remove_element(lista_interfaces, interfaz_actual);
    pthread_mutex_unlock(mutex_lista_interfaces);
}

void agregarMutexInterfazAlSistema(t_mutex_eliminacion_interfaz * nuevo_mutex){
    agregarElementoAListaSincronizada(lista_mutex_eliminacion_interfaz, nuevo_mutex);
}

char * nombre_mutex_interfaz_comparar;

t_mutex_eliminacion_interfaz * buscarMutexEliminacionLista(char * nombre_interfaz) {
    pthread_mutex_lock(lista_mutex_eliminacion_interfaz->mutex_lista);
    nombre_mutex_interfaz_comparar = string_duplicate(nombre_interfaz);
    t_mutex_eliminacion_interfaz * mutex_nuevo = list_find(lista_mutex_eliminacion_interfaz->lista, encontrarMutexEliminacionPorNombre);
    free(nombre_mutex_interfaz_comparar);
    pthread_mutex_unlock(lista_mutex_eliminacion_interfaz->mutex_lista);
    return mutex_nuevo;
}

bool encontrarMutexEliminacionPorNombre(void * elemento) {
    t_mutex_eliminacion_interfaz * mutex = (t_mutex_eliminacion_interfaz *) elemento;
    return !strcmp(mutex->nombre_interfaz, nombre_mutex_interfaz_comparar); 
}

void indicarSiEstaEnReadyComunONo(uint32_t pid, bool esReadyPlus) {
    t_estado_proceso * estado = encontrarEstadoPorPID(pid);
    estado->esta_en_ready_plus = esReadyPlus;
}