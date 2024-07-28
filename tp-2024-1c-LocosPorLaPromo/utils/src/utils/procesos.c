#include "procesos.h"
#include <commons/collections/list.h>
#include <commons/string.h>
#include <string.h>
#include <utils/logger_concurrente.h>

t_list * archivo_codigo;



void agregarProcesoABuffer(t_buffer * buffer, pcb * proceso_a_serializar);
uint32_t tamanioProceso();

t_buffer * serializarProceso(pcb * proceso_a_serializar) {
    t_buffer * buffer = crearBufferGeneral(
        tamanioProceso()
    );

    agregarProcesoABuffer(buffer, proceso_a_serializar);

    return buffer;
}


pcb * deserializarProceso(t_buffer * buffer_paquete) {
    pcb * proceso = malloc(sizeof(pcb));
    
    proceso->PID = leerDeBufferUint32(buffer_paquete);
    proceso->PC = leerDeBufferUint32(buffer_paquete);
    proceso->registros.AX = leerDeBufferUint8(buffer_paquete);
    proceso->registros.BX = leerDeBufferUint8(buffer_paquete);
    proceso->registros.CX = leerDeBufferUint8(buffer_paquete);
    proceso->registros.DX = leerDeBufferUint8(buffer_paquete);
    proceso->registros.EAX = leerDeBufferUint32(buffer_paquete);
    proceso->registros.EBX = leerDeBufferUint32(buffer_paquete);
    proceso->registros.ECX = leerDeBufferUint32(buffer_paquete);
    proceso->registros.EDX = leerDeBufferUint32(buffer_paquete);

    return proceso;
}



void enviarBufferProcesoConMotivo(t_buffer * buffer, codigos_operacion motivo, int un_socket) {
    enviarBufferPorPaquete(buffer, (uint8_t) motivo, un_socket);
}

t_buffer * serializarProcesoIOGENSLEEP(pcb_IOGENSLEEP * proceso) {
    t_buffer * buffer = crearBufferGeneral(
        tamanioProceso()
        + sizeof(uint32_t)  //longitud nombre de interfaz
        + proceso->longitud_nombre_interfaz
        + sizeof(uint32_t)  //unidades de trabajo
    );

    agregarProcesoABuffer(buffer, proceso->contexto);
    agregarABufferString(buffer, proceso->nombre_interfaz, proceso->longitud_nombre_interfaz);
    agregarABufferUint32(buffer, proceso->unidades_de_trabajo);
    return buffer;
}

pcb_IOGENSLEEP * deserealizarProcesoIOGENSLEEP(t_buffer * buffer_paquete) {
    pcb_IOGENSLEEP * pcb_iogensleep = malloc(sizeof(pcb_IOGENSLEEP));

    pcb_iogensleep->contexto = deserializarProceso(buffer_paquete);    
    pcb_iogensleep->longitud_nombre_interfaz = leerDeBufferUint32(buffer_paquete);
    pcb_iogensleep->nombre_interfaz = leerDeBufferString(buffer_paquete, pcb_iogensleep->longitud_nombre_interfaz);
    pcb_iogensleep->unidades_de_trabajo = leerDeBufferUint32(buffer_paquete);

    return pcb_iogensleep;
}

void agregarProcesoABuffer(t_buffer * buffer, pcb * proceso_a_serializar) {
    agregarABufferUint32(buffer, proceso_a_serializar->PID);
    agregarABufferUint32(buffer, proceso_a_serializar->PC);
    agregarABufferUint8(buffer, proceso_a_serializar->registros.AX);
    agregarABufferUint8(buffer, proceso_a_serializar->registros.BX);
    agregarABufferUint8(buffer, proceso_a_serializar->registros.CX);
    agregarABufferUint8(buffer, proceso_a_serializar->registros.DX);
    agregarABufferUint32(buffer, proceso_a_serializar->registros.EAX);
    agregarABufferUint32(buffer, proceso_a_serializar->registros.EBX);
    agregarABufferUint32(buffer, proceso_a_serializar->registros.ECX);
    agregarABufferUint32(buffer, proceso_a_serializar->registros.EDX);
}



t_buffer * serializarInicializarProceso(inicializar_proceso * nueva_peticion) {
     t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)   //PID
        + sizeof(uint32_t) //Quantum
        + sizeof(uint32_t) //longitud_direccion_codigo
        + nueva_peticion->longitud_direccion_codigo
    );

    agregarABufferUint32(buffer, nueva_peticion->pid);
    agregarABufferUint32(buffer, nueva_peticion->quantum);
    agregarABufferString(buffer, nueva_peticion->direccion_codigo, nueva_peticion->longitud_direccion_codigo);

    return buffer;
}

inicializar_proceso * deserializarInicializarProceso(t_buffer * buffer) {
    inicializar_proceso * nueva_peticion = malloc(sizeof(inicializar_proceso));

    nueva_peticion->pid = leerDeBufferUint32(buffer);
    nueva_peticion->quantum = leerDeBufferUint32(buffer);
    nueva_peticion->longitud_direccion_codigo = leerDeBufferUint32(buffer);
    nueva_peticion->direccion_codigo = leerDeBufferString(buffer, nueva_peticion->longitud_direccion_codigo);

    return nueva_peticion;
}

t_buffer * serializarPeticionAInstruccion(peticion_instruccion * peticion){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)   //PID
        + sizeof(uint32_t) //PC
    );

    agregarABufferUint32(buffer, peticion->pid);
    agregarABufferUint32(buffer, peticion->pc);

    return buffer;
}

peticion_instruccion * deserializarPeticionInstruccion(t_buffer * buffer_paquete){
    peticion_instruccion * peticion = malloc(sizeof(peticion_instruccion));

    peticion->pid = leerDeBufferUint32(buffer_paquete);
    peticion->pc = leerDeBufferUint32(buffer_paquete);

    return peticion;
}

t_buffer * serializarInstruccion(t_instruccion * instruccion){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)
        + instruccion->longitud
    );

    agregarABufferString(buffer, instruccion->string_instruccion, instruccion->longitud);
    
    return buffer;
}

t_instruccion * deserializarInstruccion(t_buffer * buffer_paquete){
    t_instruccion * instruccion = malloc(sizeof(t_instruccion));

    instruccion->longitud = leerDeBufferUint32(buffer_paquete);
    instruccion->string_instruccion = leerDeBufferString(buffer_paquete, instruccion->longitud);

    return instruccion;
}

t_buffer * serializarProcesoRecurso(pcb_recurso * pcb_con_recurso) {
    t_buffer * buffer = crearBufferGeneral(
        tamanioProceso()
        + sizeof(uint32_t) //longitud_nombre_recurso
        + pcb_con_recurso->longitud_nombre_recurso // nombre_recurso
    );

    agregarProcesoABuffer(buffer, pcb_con_recurso->contexto);
    agregarABufferString(buffer, pcb_con_recurso->nombre_recurso, pcb_con_recurso->longitud_nombre_recurso);

    return buffer;
}

pcb_recurso * deserializarProcesoRecurso(t_buffer * buffer_paquete) {
    pcb_recurso * pcb_con_recurso = malloc(sizeof(pcb_recurso));

    pcb_con_recurso->contexto = deserializarProceso(buffer_paquete);
    pcb_con_recurso->longitud_nombre_recurso = leerDeBufferUint32(buffer_paquete);
    pcb_con_recurso->nombre_recurso = leerDeBufferString(buffer_paquete, pcb_con_recurso->longitud_nombre_recurso);

    return pcb_con_recurso;
}

t_buffer * serializarProcesoIOSTDINOUT(pcb_IOSTD_IN_OUT * proceso){
    t_buffer * buffer = crearBufferGeneral(
        + tamanioProceso()
        + sizeof(uint32_t) //longitud_nombre_interfaz
        + proceso->longitud_nombre_interfaz //nombre_interfaz
        + sizeof(uint32_t) //cantidad_direccionesFisicas
        + proceso->cantidad_direccionesFisicas * 2 * sizeof(uint32_t) // es la cantidad de direcciones fisicas por el tamanio de cada una que serian 2 uint32_t al serializar
    );

    agregarProcesoABuffer(buffer, proceso->contexto);
    agregarABufferString(buffer, proceso->nombre_interfaz, proceso->longitud_nombre_interfaz);
    agregarABufferUint32(buffer, proceso->cantidad_direccionesFisicas);
    agregarABufferDireccionesFisicas(buffer, proceso->direccionesFisicas);    
    return buffer;
}

void agregarABufferDireccionesFisicas(t_buffer * buffer, t_queue * direccionesFisicas) {
    while(!queue_is_empty(direccionesFisicas)){
        t_direccionMemoria * direccion = (t_direccionMemoria *) queue_pop(direccionesFisicas); 
        agregarABufferUint32(buffer, direccion->direccionFisica);
        agregarABufferUint32(buffer, direccion->tamanioEnvio);
        free(direccion);
    }
}

pcb_IOSTD_IN_OUT * deserializarProcesoIOSTDINOUT(t_buffer *buffer_paquete){
    pcb_IOSTD_IN_OUT * proceso_in_out = malloc(sizeof(pcb_IOSTD_IN_OUT));
    
    proceso_in_out->contexto = deserializarProceso(buffer_paquete);
    proceso_in_out->longitud_nombre_interfaz = leerDeBufferUint32(buffer_paquete);
    proceso_in_out->nombre_interfaz = leerDeBufferString(buffer_paquete, proceso_in_out->longitud_nombre_interfaz);
    proceso_in_out->cantidad_direccionesFisicas = leerDeBufferUint32(buffer_paquete);
    proceso_in_out->direccionesFisicas = leerDeBufferDireccionesFisicas(buffer_paquete, proceso_in_out->cantidad_direccionesFisicas);
    

    return proceso_in_out;
}

t_queue * leerDeBufferDireccionesFisicas(t_buffer * buffer, uint32_t cantidad_direccionesFisicas) {
    t_queue * cola = queue_create();
    for(int i=0; i< cantidad_direccionesFisicas; i++){
        t_direccionMemoria * direccion = malloc(sizeof(t_direccionMemoria));
        direccion->direccionFisica = leerDeBufferUint32(buffer);
        direccion->tamanioEnvio = leerDeBufferUint32(buffer);
        queue_push(cola, direccion);
    }
    return cola;
}

uint32_t tamanioProceso() {
    uint32_t tamanio = sizeof(uint32_t)   //PID
                    + sizeof(uint32_t) //PC
                    + 4 * sizeof(uint8_t) //registros AX, BX, CX y DX
                    + 4 * sizeof(uint32_t); //registros EAX, EBX, ECX, EDX
    return tamanio;
}

uint32_t deserializarBorrarMemoriaP(t_buffer * buffer_paquete) { //Retorna el PID del proceso a borrar
    return leerDeBufferUint32(buffer_paquete); 
}

t_buffer * serializarPedidoMarco(int numeroPagina, int pid){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) //PID
        + sizeof(uint32_t) //Numero Pagina
    );
    agregarABufferUint32(buffer, (uint32_t) pid);
    agregarABufferUint32(buffer, (uint32_t) numeroPagina);
    return buffer;
}

t_pedidoMarco * deserializarPedidoMarco(t_buffer * buffer_paquete){
    t_pedidoMarco * pedidoMarco = malloc(sizeof(t_pedidoMarco));
    pedidoMarco->pid = leerDeBufferUint32(buffer_paquete);
    pedidoMarco->numeroPagina = leerDeBufferUint32(buffer_paquete);
    return pedidoMarco;
}

t_buffer * serializarMarco(int numeroMarco){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) //Numero Marco
    );
    agregarABufferUint32(buffer, (uint32_t) numeroMarco);
    return buffer;
}

int deserializarMarco(t_buffer *buffer_paquete){
    return (int) leerDeBufferUint32(buffer_paquete); 
}

t_operacionMemEscribirUsuario * crearOperacionMemEscribirUsuario(t_direccionMemoria * direccion, uint32_t pid, void * valor){
    t_operacionMemEscribirUsuario * operacion = malloc(sizeof(t_operacionMemEscribirUsuario));
    operacion->direccion = direccion;
    operacion->pid = pid;
    void * valorAGuardar = malloc(direccion->tamanioEnvio);
    memcpy(valorAGuardar, valor, direccion->tamanioEnvio);
    operacion->valor = valorAGuardar;
    return operacion;
}

t_operacionMemLeerUsuario * crearOperacionMemLeerUsuario(t_direccionMemoria * direccion, uint32_t pid){
    t_operacionMemLeerUsuario * operacion = malloc(sizeof(t_operacionMemLeerUsuario));
    operacion->direccion = direccion;
    operacion->pid = pid;
    return operacion;
}

t_buffer * serializarOperacionMemEscribirUsuario(t_operacionMemEscribirUsuario * operacion) {
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)   //direccionFisica
        + sizeof(uint8_t)  //tamanioEnvio
        + sizeof(uint32_t) //PID
        + operacion->direccion->tamanioEnvio //valor
    );

    agregarABufferUint32(buffer, operacion->direccion->direccionFisica);
    agregarABufferUint8(buffer, operacion->direccion->tamanioEnvio);
    agregarABufferUint32(buffer, operacion->pid);
    agregarABufferVoidP(buffer, operacion->valor, operacion->direccion->tamanioEnvio);

    return buffer;
}

t_buffer * serializarOperacionMemLeerUsuario(t_operacionMemLeerUsuario * operacion) {
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)   //direccionFisica
        + sizeof(uint8_t)  //tamanioEnvio
        + sizeof(uint32_t) //PID
    );

    agregarABufferUint32(buffer, operacion->direccion->direccionFisica);
    agregarABufferUint8(buffer, operacion->direccion->tamanioEnvio);
    agregarABufferUint32(buffer, operacion->pid);

    return buffer;
}

t_operacionMemEscribirUsuario * deserializarOperacionMemEscribirUsuario(t_buffer * buffer_paquete) {
    t_operacionMemEscribirUsuario * operacion = malloc(sizeof(t_operacionMemEscribirUsuario));
    operacion->direccion = malloc(sizeof(t_direccionMemoria));
    operacion->direccion->direccionFisica = leerDeBufferUint32(buffer_paquete);
    operacion->direccion->tamanioEnvio = leerDeBufferUint8(buffer_paquete);
    operacion->pid = leerDeBufferUint32(buffer_paquete);
    operacion->valor = leerDeBufferVoidP(buffer_paquete, operacion->direccion->tamanioEnvio);

    return operacion;
}

t_operacionMemLeerUsuario * deserializarOperacionMemLeerUsuario(t_buffer * buffer_paquete) {
    t_operacionMemLeerUsuario * operacion = malloc(sizeof(t_operacionMemLeerUsuario));
    operacion->direccion = malloc(sizeof(t_direccionMemoria));
    operacion->direccion->direccionFisica = leerDeBufferUint32(buffer_paquete);
    operacion->direccion->tamanioEnvio = leerDeBufferUint8(buffer_paquete);
    operacion->pid = leerDeBufferUint32(buffer_paquete);

    return operacion;
}

void hacerOperacionEscrituraMemoria(t_queue * direccionesFisicas, void * valor, uint32_t pid, int conexionMemoria) {
    uint32_t offset = 0;
    while(!queue_is_empty(direccionesFisicas)){
        t_direccionMemoria * direccion = (t_direccionMemoria *) queue_pop(direccionesFisicas);
        t_operacionMemEscribirUsuario * operacion = crearOperacionMemEscribirUsuario(direccion, pid, valor + offset);
        t_buffer * buffer = serializarOperacionMemEscribirUsuario(operacion);
        enviarBufferPorPaquete(buffer, ESCRIBIR_MEMORIA, conexionMemoria);
        t_paquete * paquete = recibirPaqueteGeneral(conexionMemoria);
        eliminar_paquete(paquete);
        offset += direccion->tamanioEnvio;
        free(direccion);
        free(operacion->valor);
        free(operacion);
    }
}

void * hacerOperacionLecturaMemoria(t_queue * direccionesFisicas, void * valor, uint32_t pid, int conexionMemoria){
    uint32_t offset = 0;
    while(!queue_is_empty(direccionesFisicas)){
        t_direccionMemoria * direccion = (t_direccionMemoria *) queue_pop(direccionesFisicas);
        t_operacionMemLeerUsuario * operacion = crearOperacionMemLeerUsuario(direccion, pid);
        t_buffer * buffer = serializarOperacionMemLeerUsuario(operacion);
        enviarBufferPorPaquete(buffer, LEER_MEMORIA, conexionMemoria);
        t_paquete * paquete_repuesta = recibirPaqueteGeneral(conexionMemoria);
        memcpy(valor + offset, paquete_repuesta->buffer->stream, direccion->tamanioEnvio);
        offset += direccion->tamanioEnvio;
        eliminar_paquete(paquete_repuesta);
        free(direccion);
        free(operacion);
    }
    return valor;
}

t_buffer * serializarPeticionResize(peticion_resize * peticion){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) // PID
        + sizeof(uint32_t)// Tamanio
    );

    agregarABufferUint32(buffer, peticion->pid);
    agregarABufferUint32(buffer, peticion->tamanio);

    return buffer;
}

peticion_resize * deserializarPeticionResize(t_buffer * buffer_paquete){
    peticion_resize * peticion = malloc(sizeof(peticion_resize));
    peticion->pid = leerDeBufferUint32(buffer_paquete);
    peticion->tamanio = leerDeBufferUint32(buffer_paquete);

    return peticion;
}

int deserializarTamPagina(t_buffer * buffer_paquete){ return (int) leerDeBufferUint32(buffer_paquete); }

t_buffer * serializarTamPagina(uint32_t tamanio_pagina){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) //tamanio_pagina
    );

    agregarABufferUint32(buffer, tamanio_pagina);
    return buffer;
}


t_buffer * serializarProcesoFSCREADEL(pcb_FSCREADEL * proceso_a_enviar){
    t_buffer * buffer = crearBufferGeneral(
        tamanioProceso()
        + sizeof(uint32_t)                           //longitud nombre interfaz
        + proceso_a_enviar->longitud_nombre_interfaz //nombre interfaz
        + sizeof(uint32_t)                           //longitud nombre archivo
        + proceso_a_enviar->longitud_nombre_archivo  //nombre archivo
    );

    agregarProcesoABuffer(buffer, proceso_a_enviar->contexto);
    agregarABufferString(buffer, proceso_a_enviar->nombre_interfaz, proceso_a_enviar->longitud_nombre_interfaz);
    agregarABufferString(buffer, proceso_a_enviar->nombre_archivo, proceso_a_enviar->longitud_nombre_archivo);

    return buffer;
}

pcb_FSCREADEL * deserializarProcesoFSCREADEL(t_buffer * buffer_paquete) {
    pcb_FSCREADEL * pcb_nuevo = malloc(sizeof(pcb_FSCREADEL));
    
    pcb_nuevo->contexto = deserializarProceso(buffer_paquete);
    pcb_nuevo->longitud_nombre_interfaz = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->nombre_interfaz = leerDeBufferString(buffer_paquete, pcb_nuevo->longitud_nombre_interfaz);
    pcb_nuevo->longitud_nombre_archivo = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->nombre_archivo = leerDeBufferString(buffer_paquete, pcb_nuevo->longitud_nombre_archivo);

    return pcb_nuevo;
}

t_buffer * serializarProcesoFSTRUN(pcb_FSTRUN * proceso_a_enviar) {
    t_buffer * buffer = crearBufferGeneral(
        tamanioProceso()
        + sizeof(uint32_t)                           //longitud nombre interfaz
        + proceso_a_enviar->longitud_nombre_interfaz //nombre interfaz
        + sizeof(uint32_t)                           //longitud nombre archivo
        + proceso_a_enviar->longitud_nombre_archivo  //nombre archivo
        + sizeof(uint32_t)                           //tamanio nuevo archivo
    );

    agregarProcesoABuffer(buffer, proceso_a_enviar->contexto);
    agregarABufferString(buffer, proceso_a_enviar->nombre_interfaz, proceso_a_enviar->longitud_nombre_interfaz);
    agregarABufferString(buffer, proceso_a_enviar->nombre_archivo, proceso_a_enviar->longitud_nombre_archivo);
    agregarABufferUint32(buffer, proceso_a_enviar->tamanio_nuevo_archivo);

    return buffer;
}

pcb_FSTRUN * deserializarProcesoFSTRUN(t_buffer * buffer_paquete) {
    pcb_FSTRUN * pcb_nuevo = malloc(sizeof(pcb_FSTRUN));
    
    pcb_nuevo->contexto = deserializarProceso(buffer_paquete);
    pcb_nuevo->longitud_nombre_interfaz = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->nombre_interfaz = leerDeBufferString(buffer_paquete, pcb_nuevo->longitud_nombre_interfaz);
    pcb_nuevo->longitud_nombre_archivo = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->nombre_archivo = leerDeBufferString(buffer_paquete, pcb_nuevo->longitud_nombre_archivo);
    pcb_nuevo->tamanio_nuevo_archivo = leerDeBufferUint32(buffer_paquete);

    return pcb_nuevo;
}

t_buffer * serializarProcesoFSWR(pcb_FSWR * proceso_a_enviar) {
    t_buffer * buffer = crearBufferGeneral(
        tamanioProceso()
        + sizeof(uint32_t)                           //longitud nombre interfaz
        + proceso_a_enviar->longitud_nombre_interfaz //nombre interfaz
        + sizeof(uint32_t)                           //longitud nombre archivo
        + proceso_a_enviar->longitud_nombre_archivo  //nombre archivo
        + sizeof(uint32_t)                           //punteroArchivo
        + sizeof(uint32_t)                           //cantidad_direccionesFisicas
        + proceso_a_enviar->cantidad_direccionesFisicas*2*sizeof(uint32_t) //direccionesFisicas
    );

    agregarProcesoABuffer(buffer, proceso_a_enviar->contexto);
    agregarABufferString(buffer, proceso_a_enviar->nombre_interfaz, proceso_a_enviar->longitud_nombre_interfaz);
    agregarABufferString(buffer, proceso_a_enviar->nombre_archivo, proceso_a_enviar->longitud_nombre_archivo);
    agregarABufferUint32(buffer, proceso_a_enviar->punteroArchivo);
    agregarABufferUint32(buffer, proceso_a_enviar->cantidad_direccionesFisicas);
    agregarABufferDireccionesFisicas(buffer, proceso_a_enviar->direccionesFisicas);

    return buffer;
}

pcb_FSWR * deserializarProcesoFSWR(t_buffer * buffer_paquete) {
    pcb_FSWR * pcb_nuevo = malloc(sizeof(pcb_FSWR));
    
    pcb_nuevo->contexto = deserializarProceso(buffer_paquete);
    pcb_nuevo->longitud_nombre_interfaz = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->nombre_interfaz = leerDeBufferString(buffer_paquete, pcb_nuevo->longitud_nombre_interfaz);
    pcb_nuevo->longitud_nombre_archivo = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->nombre_archivo = leerDeBufferString(buffer_paquete, pcb_nuevo->longitud_nombre_archivo);
    pcb_nuevo->punteroArchivo = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->cantidad_direccionesFisicas = leerDeBufferUint32(buffer_paquete);
    pcb_nuevo->direccionesFisicas = leerDeBufferDireccionesFisicas(buffer_paquete, pcb_nuevo->cantidad_direccionesFisicas);

    return pcb_nuevo;
}