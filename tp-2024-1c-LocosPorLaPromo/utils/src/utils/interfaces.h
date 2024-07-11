#ifndef INTERFACES_H_
#define INTERFACES_H_

#include<netdb.h>
#include <utils/conexiones.h>
#include <commons/collections/queue.h>
#include <utils/procesos.h>

typedef struct {
    uint32_t longitud_nombre;
    char * nombre;
    uint8_t tipo;
} info_interfaz;

typedef enum{
    GENERICA,
    STDIN,
    STDOUT,
    DIALFS    
}t_interfaz; 

typedef struct {
    uint32_t pid;
    uint32_t cantidad_direccionesFisicas;
    t_queue * direccionesFisicas;
} operacionSTDInOut;

typedef struct {
    uint32_t pid;
    uint32_t unidades_de_trabajo;
} operacionIOGENSLEEP;

typedef struct {
    uint32_t pid;
    uint32_t longitud_nombre_archivo;
    char * nombre_archivo;
} operacionFSCREADEL;

typedef struct {
    uint32_t pid;
    uint32_t longitud_nombre_archivo;
    char * nombre_archivo;
    uint32_t tamanio_archivo;
} operacionFSTRUN;

typedef struct {
    uint32_t pid;
    uint32_t longitud_nombre_archivo;
    char * nombre_archivo;
    uint32_t punteroArchivo;
    uint32_t cantidad_direccionesFisicas;
    t_queue * direccionesFisicas;
} operacionFSWR;

t_buffer * serializarInformacionInterfaz(info_interfaz * interfaz);

t_buffer * serializarOperacionIOGENSLEEP(pcb_cola_interfaz * pcb_a_trabajar);

operacionIOGENSLEEP * deserializarOperacionIOGENSLEEP(t_buffer * buffer_operacion); 

t_buffer * serializarOperacionSTDInOut(pcb_cola_interfaz * pcb_a_trabajar);

operacionSTDInOut * deserializarOperacionSTDInOut(t_buffer * buffer_operacion);

t_buffer * serializarOperacionFSCREADEL(pcb_cola_interfaz * pcb_a_trabajar);

operacionFSCREADEL * deserializarOperacionFSCREADEL(t_buffer * buffer_paquete);

t_buffer * serializarOperacionFSTRUN(pcb_cola_interfaz * pcb_a_trabajar);

operacionFSTRUN * deserializarOperacionFSTRUN(t_buffer * buffer_paquete);

t_buffer * serializarOperacionFSWR(pcb_cola_interfaz * pcb_a_trabajar);

operacionFSWR * deserializarOperacionFSWR(t_buffer * buffer_paquete);
#endif