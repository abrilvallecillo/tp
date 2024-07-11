#ifndef ATENCION_MODULOS_H_
#define ATENCION_MODULOS_H_

#include "memoria.h"
#include <utils/procesos.h>

void * atenderPeticiones(void * conexion_cliente);
char * buscarInstruccion(peticion_instruccion * nueva_instruccion);
bool pid_a_comparar(void* codigo_del_programa);
pcb * CrearProceso(inicializar_proceso * peticion);
t_instruccion * estandarizarStringInstruccion(char * string_instruccion);
void enviarSegunErrorAlBuscarInstr(t_buffer * buffer, char * string_instruccion, int socket_cliente);


typedef struct {
    int pid;
    char ** array_instrucciones;
} codigo_proceso; 

#endif
