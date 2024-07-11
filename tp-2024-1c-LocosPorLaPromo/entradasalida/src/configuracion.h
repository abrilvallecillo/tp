#ifndef _CPU_CONFIG_
#define _CPU_CONFIG_

#include <commons/config.h>
#include <commons/log.h>

// Definición de una estructura para almacenar la configuración de la entradasalida
typedef struct {
  char* TIPO_INTERFAZ;
  int TIEMPO_UNIDAD_TRABAJO;

  char* IP_KERNEL;
  char* PUERTO_KERNEL;

  char* IP_MEMORIA;
  char* PUERTO_MEMORIA;

  char* PATH_BASE_DIALFS;
  int BLOCK_SIZE;
  int BLOCK_COUNT;
  int RETRASO_COMPACTACION;
} t_cfg;

extern t_cfg configuracion;
extern t_config* config;

// Declaración de funciones
void obtener_config(char * direccion);

#endif
