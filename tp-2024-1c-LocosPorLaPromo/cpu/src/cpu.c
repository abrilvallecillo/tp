#include "cpu.h"
#include <utils/procesos.h>
#include <utils/logger_concurrente.h>
#include <commons/string.h>
#include "tlb.h"

// -------------------------- Definiciones --------------------------

registros_CPU registrosPropios;

int conexion_dispatch_kernel;
entero_sincronizado * estado_interrupcion;
t_queue * cola_parametros;
int conexion_memoria;
int interrupcion_proceso;
entero_sincronizado * pid_a_finalizar;
uint32_t pid_actual;
int tamanioPagina;
entero_sincronizado * consumirFinQ;
bool incrementaPC;

// -------------------------- Carga de Registros --------------------------

void cargarRegistrosEnCPU(pcb * proceso_elegido){
    registrosPropios.generales.AX = proceso_elegido->registros.AX;
    registrosPropios.generales.BX = proceso_elegido->registros.BX;
    registrosPropios.generales.CX = proceso_elegido->registros.CX;
    registrosPropios.generales.DX = proceso_elegido->registros.DX;
    registrosPropios.generales.EAX = proceso_elegido->registros.EAX;
    registrosPropios.generales.EBX = proceso_elegido->registros.EBX;
    registrosPropios.generales.ECX = proceso_elegido->registros.ECX;
    registrosPropios.generales.EDX = proceso_elegido->registros.EDX;
    registrosPropios.PC = proceso_elegido->PC;
};

void cargarRegistrosEnPCB(pcb * proceso_ejecutado){
    proceso_ejecutado->PC = registrosPropios.PC;
    proceso_ejecutado->registros.AX = registrosPropios.generales.AX;
    proceso_ejecutado->registros.BX = registrosPropios.generales.BX;
    proceso_ejecutado->registros.CX = registrosPropios.generales.CX;
    proceso_ejecutado->registros.DX = registrosPropios.generales.DX;
    proceso_ejecutado->registros.EAX = registrosPropios.generales.EAX;
    proceso_ejecutado->registros.EBX = registrosPropios.generales.EBX;
    proceso_ejecutado->registros.ECX = registrosPropios.generales.ECX;
    proceso_ejecutado->registros.EDX = registrosPropios.generales.EDX;
}

// -------------------------- MMU --------------------------

// direcciones lógicas = [número_pagina | desplazamiento]

// número_página = floor(dirección_lógica / tamaño_página)
// desplazamiento = dirección_lógica - número_página * tamaño_página

// Es importante tener en cuenta en este punto que una Dirección Lógica va a pertenecer a una página en cuestión, pero el contenido a leer o escribir puede ser que ocupe más de una página.

t_queue * traducirMemoria(int direccionLogica, int tamanioDato){
    t_queue * colaDireccionesFisicas = queue_create();

    int numeroPagina = (int)(direccionLogica / tamanioPagina);
    int desplazamiento = direccionLogica - numeroPagina * tamanioPagina;
    
    int espacioPaginaLibre = tamanioPagina - desplazamiento;
    int tamanioAEnviar = minInt(tamanioDato, espacioPaginaLibre);
    int tamanioInstantaneo = tamanioDato - tamanioAEnviar; // lo que me queda
    
    int i = 1;

    t_direccionMemoria * direccionFisica = direccionLogicaAFisica(numeroPagina, tamanioAEnviar, pid_actual);
    if(direccionFisica == NULL){
        queue_destroy(colaDireccionesFisicas); 
        return NULL;
    } 
    direccionFisica->direccionFisica += desplazamiento;
    queue_push(colaDireccionesFisicas, direccionFisica);

    while(tamanioInstantaneo){
        tamanioAEnviar = minInt(tamanioInstantaneo, tamanioPagina);
        tamanioInstantaneo -= tamanioAEnviar;
        direccionFisica = direccionLogicaAFisica(numeroPagina + i, tamanioAEnviar, pid_actual);
        if(direccionFisica == NULL){
            queue_destroy_and_destroy_elements(colaDireccionesFisicas, free);
            return NULL;
        } 
        queue_push(colaDireccionesFisicas, direccionFisica);
        i++;
    } return colaDireccionesFisicas;
}

int minInt(int entero1, int entero2){
    if(entero1 < entero2) return entero1;
    return entero2;
}

t_direccionMemoria * direccionLogicaAFisica(int numeroPagina, int tamanioAEnviar, int pid){
    t_direccionMemoria * direccionMemoria = malloc(sizeof(t_direccionMemoria));
    
    int respuestaTLB = buscarMarcoEnTLB(numeroPagina, pid);

    if(respuestaTLB != -1){
        
        direccionMemoria->direccionFisica = respuestaTLB * tamanioPagina;
        direccionMemoria->tamanioEnvio = tamanioAEnviar;

        char * mensaje = string_from_format ("PID: %d - TLB HIT - Pagina: %d", pid, numeroPagina);
        logInfoSincronizado(mensaje);
        free(mensaje);

        mensaje = string_from_format ("PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, numeroPagina, respuestaTLB);
        logInfoSincronizado(mensaje);
        free(mensaje);
   
    } else {

        char * mensaje = string_from_format ("PID: %d - TLB MISS - Pagina:  %d", pid, numeroPagina);
        logInfoSincronizado(mensaje);
        free(mensaje);
        
        solicitarMarcoAMemoria(numeroPagina, pid);
        int marcoMemoria = recibirMarcoDeMemoria();
       
        if(marcoMemoria == -1) {
            free(direccionMemoria);
            return NULL;
        }

        direccionMemoria->direccionFisica = marcoMemoria * tamanioPagina;
        direccionMemoria->tamanioEnvio = tamanioAEnviar;

        agregarDireccionATLB(pid, numeroPagina, marcoMemoria);

        mensaje = string_from_format ("PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, numeroPagina, marcoMemoria);
        logInfoSincronizado(mensaje);
        free(mensaje);

    } return direccionMemoria;
}

void solicitarMarcoAMemoria(int numeroPagina, int pid){
    t_buffer * buffer = serializarPedidoMarco(numeroPagina, pid);
    enviarBufferPorPaquete(buffer, PEDIDO_MARCO, conexion_memoria);
}

int recibirMarcoDeMemoria(){
    t_paquete * paqueteMarco = recibirPaqueteGeneral(conexion_memoria);
    if(paqueteMarco->codigoOperacion != PEDIDO_MARCO) {
        eliminar_paquete(paqueteMarco);
        return -1;
    }
    int marco = deserializarMarco(paqueteMarco->buffer);
    eliminar_paquete(paqueteMarco);
    return marco;
}

// -------------------------- X --------------------------

pcb_recurso * crearPCBRecurso(pcb * proceso_elegido){
    pcb_recurso * proceso_a_enviar = malloc(sizeof(pcb_recurso));
    proceso_a_enviar->contexto = proceso_elegido;
    char * recurso_cargado = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_recurso = string_duplicate(recurso_cargado);
    proceso_a_enviar->longitud_nombre_recurso = string_length(recurso_cargado)+1;
    free(recurso_cargado);
    return proceso_a_enviar;
}

pcb_IOGENSLEEP * crearPCB_IOGENSLEEP(pcb * proceso_elegido){
    pcb_IOGENSLEEP * proceso_a_enviar = malloc(sizeof(pcb_IOGENSLEEP));
    proceso_a_enviar->contexto = proceso_elegido;
    char * interfaz_cargada = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_interfaz = string_duplicate(interfaz_cargada);
    proceso_a_enviar->longitud_nombre_interfaz = string_length(interfaz_cargada)+1;
    free(interfaz_cargada);
    int * unidades_de_tiempo_cargar = (int *) queue_pop(cola_parametros);
    proceso_a_enviar->unidades_de_trabajo = *unidades_de_tiempo_cargar;
    free(unidades_de_tiempo_cargar);
    return proceso_a_enviar;
}

pcb_IOSTD_IN_OUT * crearPCB_STDINOUT(pcb * proceso_elegido) {
    pcb_IOSTD_IN_OUT * proceso_a_enviar = malloc(sizeof(pcb_IOSTD_IN_OUT));
    proceso_a_enviar->contexto = proceso_elegido;
    char * interfaz_cargada = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_interfaz = string_duplicate(interfaz_cargada);
    proceso_a_enviar->longitud_nombre_interfaz = string_length(interfaz_cargada)+1;
    free(interfaz_cargada);
    t_queue * direccionesFisicas = (t_queue *) queue_pop(cola_parametros);
    proceso_a_enviar->direccionesFisicas = direccionesFisicas;
    proceso_a_enviar->cantidad_direccionesFisicas = queue_size(direccionesFisicas);
    return proceso_a_enviar;
}

pcb_FSCREADEL * crearPCB_FSCREADEL(pcb * proceso_elegido) {
    pcb_FSCREADEL * proceso_a_enviar = malloc(sizeof(pcb_FSCREADEL));
    proceso_a_enviar->contexto = proceso_elegido;
    char * interfaz_cargada = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_interfaz = string_duplicate(interfaz_cargada);
    proceso_a_enviar->longitud_nombre_interfaz = string_length(interfaz_cargada)+1;
    free(interfaz_cargada);
    char * nombre_archivo = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_archivo = string_duplicate(nombre_archivo);
    proceso_a_enviar->longitud_nombre_archivo = string_length(nombre_archivo)+1;
    free(nombre_archivo);
    return proceso_a_enviar;
}

pcb_FSTRUN * crearPCB_FSTRUN(pcb * proceso_elegido) {
    pcb_FSTRUN * proceso_a_enviar = malloc(sizeof(pcb_FSTRUN));
    proceso_a_enviar->contexto = proceso_elegido;
    char * interfaz_cargada = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_interfaz = string_duplicate(interfaz_cargada);
    proceso_a_enviar->longitud_nombre_interfaz = string_length(interfaz_cargada)+1;
    free(interfaz_cargada);
    char * nombre_archivo = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_archivo = string_duplicate(nombre_archivo);
    proceso_a_enviar->longitud_nombre_archivo = string_length(nombre_archivo)+1;
    free(nombre_archivo);
    int * tamanioNuevo = (int *) queue_pop(cola_parametros);
    proceso_a_enviar->tamanio_nuevo_archivo = *tamanioNuevo;
    free(tamanioNuevo);
    return proceso_a_enviar;
}

pcb_FSWR * crearPCB_FSWR(pcb * proceso_elegido) {
    pcb_FSWR * proceso_a_enviar = malloc(sizeof(pcb_FSWR));
    proceso_a_enviar->contexto = proceso_elegido;
    char * interfaz_cargada = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_interfaz = string_duplicate(interfaz_cargada);
    proceso_a_enviar->longitud_nombre_interfaz = string_length(interfaz_cargada)+1;
    free(interfaz_cargada);
    char * nombre_archivo = (char *) queue_pop(cola_parametros);
    proceso_a_enviar->nombre_archivo = string_duplicate(nombre_archivo);
    proceso_a_enviar->longitud_nombre_archivo = string_length(nombre_archivo)+1;
    free(nombre_archivo);
    int * punteroArchivo = (int *) queue_pop(cola_parametros);
    proceso_a_enviar->punteroArchivo = *punteroArchivo;
    free(punteroArchivo);
    t_queue * direccionesFisicas = (t_queue *) queue_pop(cola_parametros);
    proceso_a_enviar->direccionesFisicas = direccionesFisicas;
    proceso_a_enviar->cantidad_direccionesFisicas = queue_size(direccionesFisicas);
    return proceso_a_enviar;
}

// -------------------------- Operaciones Registros Casteados --------------------------

registro_casteado * crearRegistroCasteado(bool es32bits, void * direccion_registro) {
    registro_casteado * registro = malloc(sizeof(registro_casteado));
    registro->es32bits = es32bits;
    registro->registro = direccion_registro; 
    return registro; 
}

void modificarValorRegistroCasteado(registro_casteado * registro_cast, int valor) {
    if(registro_cast->es32bits) {
        uint32_t * registro32 = (uint32_t *) registro_cast->registro;
        *registro32 = (uint32_t) valor;
    } else {
        uint8_t * registro8 = (uint8_t *) registro_cast->registro;
        *registro8 = (uint8_t) valor;
    }
}

int leerValorRegistroCasteado(registro_casteado * registro_cast){
    if(registro_cast->es32bits) {
        uint32_t * registro32 = (uint32_t *) registro_cast->registro;
        return (int) *registro32;
    } else {
        uint8_t * registro8 = (uint8_t *) registro_cast->registro;
        return (int) *registro8;
   } 
}

int tamanioRegistroCasteado(registro_casteado * registro_cast) {
    if(registro_cast->es32bits) return sizeof(uint32_t); 
    return sizeof(uint8_t);
}

// ----------------------------------------------------

bool esInterrupcionSincronica(int interrupcion){
    return (
        interrupcion == P_WAIT 
        || interrupcion == P_SIGNAL
        || interrupcion == P_IO_GEN_SLEEP 
        || interrupcion == P_IO_STDIN_READ
        || interrupcion == P_IO_STDOUT_WRITE 
        || interrupcion == P_IO_FS_CREATE
        || interrupcion == P_IO_FS_DELETE 
        || interrupcion == P_IO_FS_READ
        || interrupcion == P_IO_FS_WRITE 
        || interrupcion == P_IO_FS_TRUNCATE
        || interrupcion == P_EXIT
        || interrupcion == OUT_OF_MEMORY
        );
}
