#ifndef MANEJO_MEMORIA_H_
#define MANEJO_MEMORIA_H_

void *manejarMemoria(void * sin_parametro);
pcb * recibirRespuestaACrearProceso(int conexion_memoria);
inicializar_proceso * crearInicializarProceso(peticion_memoria * nueva_peticion);
t_buffer * serializarBorrarMemoriaP(peticion_memoria * nueva_peticion);
void cargarProcesoEnSistema(uint32_t pid);
#endif