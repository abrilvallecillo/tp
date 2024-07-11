#include "interfaces.h"
#include <utils/procesos.h>
#include <string.h>


t_buffer * serializarInformacionInterfaz(info_interfaz * interfaz){
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) //longitud_nombre
        + interfaz->longitud_nombre
        + sizeof(uint8_t) //tipo_interfaz
    );

    agregarABufferString(buffer, interfaz->nombre, interfaz->longitud_nombre);
    agregarABufferUint8(buffer, interfaz->tipo);

    return buffer;
}

t_buffer * serializarOperacionIOGENSLEEP(pcb_cola_interfaz * pcb_a_trabajar) {
    t_buffer *buffer = crearBufferGeneral(
        sizeof(uint32_t)   //PID
        + sizeof(uint32_t) //unidades_de_trabajo 
    );
    uint32_t * unidades_de_trabajo = (uint32_t *) queue_pop(pcb_a_trabajar->cola_parametros);

    agregarABufferUint32(buffer, pcb_a_trabajar->contexto->PID);
    agregarABufferUint32(buffer, *unidades_de_trabajo);
    
    free(unidades_de_trabajo);
    return buffer;
}

operacionIOGENSLEEP * deserializarOperacionIOGENSLEEP(t_buffer * buffer_operacion) {
    operacionIOGENSLEEP * operacion = malloc(sizeof(operacionIOGENSLEEP));
    operacion->pid = leerDeBufferUint32(buffer_operacion);
    operacion->unidades_de_trabajo = leerDeBufferUint32(buffer_operacion);
    return operacion;
}

t_buffer * serializarOperacionSTDInOut(pcb_cola_interfaz * pcb_a_trabajar){
    uint32_t * cantidad_direccionesFisicas = (uint32_t *) queue_pop(pcb_a_trabajar->cola_parametros);
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)                                      //PID
        + sizeof(uint32_t)                                    //cantidad_direccionesFisicas
        + *cantidad_direccionesFisicas * 2 * sizeof(uint32_t) //direccionesFisicas
    );
    
    agregarABufferUint32(buffer, pcb_a_trabajar->contexto->PID);
    agregarABufferUint32(buffer, *cantidad_direccionesFisicas);
    t_queue * direccionesFisicas = (t_queue *) queue_pop(pcb_a_trabajar->cola_parametros);
    agregarABufferDireccionesFisicas(buffer, direccionesFisicas);
    
    queue_destroy(direccionesFisicas);
    free(cantidad_direccionesFisicas);
    return buffer;
}

operacionSTDInOut * deserializarOperacionSTDInOut(t_buffer * buffer_operacion) {
    operacionSTDInOut * operacion = malloc(sizeof(operacionSTDInOut));

    operacion->pid = leerDeBufferUint32(buffer_operacion);
    operacion->cantidad_direccionesFisicas = leerDeBufferUint32(buffer_operacion);
    operacion->direccionesFisicas = leerDeBufferDireccionesFisicas(buffer_operacion, operacion->cantidad_direccionesFisicas);

    return operacion;
}

t_buffer * serializarOperacionFSCREADEL(pcb_cola_interfaz * pcb_a_trabajar) {
    char * nombre_archivo = (char *) queue_pop(pcb_a_trabajar->cola_parametros);
    uint32_t longitud_nombre = strlen(nombre_archivo) + 1;
    
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)    //PID
        + sizeof(uint32_t)  //longitud_nombre_archivo
        + longitud_nombre   //nombre_archivo
    );

    agregarABufferUint32(buffer, pcb_a_trabajar->contexto->PID);
    agregarABufferString(buffer, nombre_archivo, longitud_nombre);
    free(nombre_archivo);
    return buffer;
}

operacionFSCREADEL * deserializarOperacionFSCREADEL(t_buffer * buffer_paquete) {
    operacionFSCREADEL * operacion = malloc(sizeof(operacionFSCREADEL));
    
    operacion->pid = leerDeBufferUint32(buffer_paquete);
    operacion->longitud_nombre_archivo = leerDeBufferUint32(buffer_paquete);
    operacion->nombre_archivo = leerDeBufferString(buffer_paquete, operacion->longitud_nombre_archivo);

    return operacion;
} 

t_buffer * serializarOperacionFSTRUN(pcb_cola_interfaz * pcb_a_trabajar) {
    char * nombre_archivo = (char *) queue_pop(pcb_a_trabajar->cola_parametros);
    uint32_t longitud_nombre = strlen(nombre_archivo) + 1;
    uint32_t * tamanio_archivo = (uint32_t *) queue_pop(pcb_a_trabajar->cola_parametros);
    
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)    //PID
        + sizeof(uint32_t)    //longitud_nombre_archivo
        + longitud_nombre   //nombre_archivo
        + sizeof(uint32_t)  //tamanio_archivo
    );

    agregarABufferUint32(buffer, pcb_a_trabajar->contexto->PID);
    agregarABufferString(buffer, nombre_archivo, longitud_nombre);
    agregarABufferUint32(buffer, *tamanio_archivo);
    
    free(nombre_archivo);
    free(tamanio_archivo);
    return buffer;
}

operacionFSTRUN * deserializarOperacionFSTRUN(t_buffer * buffer_paquete) {
    operacionFSTRUN * operacion = malloc(sizeof(operacionFSTRUN));
    
    operacion->pid = leerDeBufferUint32(buffer_paquete);
    operacion->longitud_nombre_archivo = leerDeBufferUint32(buffer_paquete);
    operacion->nombre_archivo = leerDeBufferString(buffer_paquete, operacion->longitud_nombre_archivo);
    operacion->tamanio_archivo = leerDeBufferUint32(buffer_paquete);

    return operacion;
} 

t_buffer * serializarOperacionFSWR(pcb_cola_interfaz * pcb_a_trabajar) {
    char * nombre_archivo = (char *) queue_pop(pcb_a_trabajar->cola_parametros);
    uint32_t longitud_nombre = strlen(nombre_archivo) + 1;
    uint32_t * punteroArchivo = (uint32_t *) queue_pop(pcb_a_trabajar->cola_parametros);
    uint32_t * cantidad_direccionesFisicas = (uint32_t *) queue_pop(pcb_a_trabajar->cola_parametros);

    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t)                                      //PID
        + sizeof(uint32_t)                                    //longitud_nombre_archivo
        + longitud_nombre                                     //nombre_archivo
        + sizeof(uint32_t)                                    //punteroArchivo
        + sizeof(uint32_t)                                    //cantidad_direccionesFisicas
        + *cantidad_direccionesFisicas * 2 * sizeof(uint32_t) //direccionesFisicas
    );
    
    agregarABufferUint32(buffer, pcb_a_trabajar->contexto->PID);
    agregarABufferString(buffer, nombre_archivo, longitud_nombre);
    agregarABufferUint32(buffer, *punteroArchivo);
    agregarABufferUint32(buffer, *cantidad_direccionesFisicas);
    t_queue * direccionesFisicas = (t_queue *) queue_pop(pcb_a_trabajar->cola_parametros);
    agregarABufferDireccionesFisicas(buffer, direccionesFisicas);
    
    queue_destroy(direccionesFisicas);
    free(nombre_archivo);
    free(punteroArchivo);
    free(cantidad_direccionesFisicas);
    return buffer;
}

operacionFSWR * deserializarOperacionFSWR(t_buffer * buffer_paquete) {
    operacionFSWR * operacion = malloc(sizeof(operacionFSWR));
    
    operacion->pid = leerDeBufferUint32(buffer_paquete);
    operacion->longitud_nombre_archivo = leerDeBufferUint32(buffer_paquete);
    operacion->nombre_archivo = leerDeBufferString(buffer_paquete, operacion->longitud_nombre_archivo);
    operacion->punteroArchivo = leerDeBufferUint32(buffer_paquete);
    operacion->cantidad_direccionesFisicas = leerDeBufferUint32(buffer_paquete);
    operacion->direccionesFisicas = leerDeBufferDireccionesFisicas(buffer_paquete, operacion->cantidad_direccionesFisicas);

    return operacion;
} 