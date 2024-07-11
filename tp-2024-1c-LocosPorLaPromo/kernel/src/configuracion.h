#ifndef _CPU_CONFIG_
#define _CPU_CONFIG_

#include <commons/config.h>
#include <commons/log.h>

// Definición de una estructura para almacenar la configuración del Kernel
typedef struct {
  char * PUERTO_ESCUCHA;

  char * IP_MEMORIA;
  char * PUERTO_MEMORIA;

  char * IP_CPU;
  char * PUERTO_CPU_DISPATCH;
  char * PUERTO_CPU_INTERRUPT;
  
  char* ALGORITMO_PLANIFICACION;
  int QUANTUM;
  char ** RECURSOS;
  char ** INSTANCIAS_RECURSOS;
  int GRADO_MULTIPROGRAMACION;
} t_cfg;

extern t_cfg configuracion;
extern t_config* config;

// Declaración de funciones
void obtener_config(char * direccion_config);

#endif
