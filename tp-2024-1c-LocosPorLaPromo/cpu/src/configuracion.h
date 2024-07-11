#ifndef _CPU_CONFIG_
#define _CPU_CONFIG_

#include <commons/config.h>
#include <commons/log.h>

// Definición de una estructura para almacenar la configuración de la CPU
typedef struct {
  char* IP_MEMORIA; 
  char* PUERTO_MEMORIA;

  char* PUERTO_ESCUCHA_DISPATCH; 
  char* PUERTO_ESCUCHA_INTERRUPT;

  int CANTIDAD_ENTRADAS_TLB;
  char* ALGORITMO_TLB;
} t_cfg;

extern t_cfg configuracion;
extern t_config* config;

// Declaración de funciones
void obtener_config(char * direccion);

#endif
