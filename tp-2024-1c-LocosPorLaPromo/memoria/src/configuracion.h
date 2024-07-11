#ifndef _MEMORIA_CONFIG_
#define _MEMORIA_CONFIG_

#include <commons/config.h>
#include <commons/log.h>

// Definición de una estructura para almacenar la configuración de la memoria
typedef struct {
  char* PUERTO; 
  int TAMANIO;
  int TAMANIO_PAGINAS; 
  char* PATH_INSTRUCCIONES; 
  int RETARDO_RESPUESTA; 
} t_cfg;

extern pthread_mutex_t mutex_log; // Mutex para sincronización en registros

extern t_cfg configuracion;
extern t_config* config;

// Declaración de funciones
void obtener_config(char * direccion);
#endif
