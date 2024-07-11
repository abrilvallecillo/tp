#include "configuracion.h" 

t_config* config; 
t_cfg configuracion; 

void obtener_config(char * direccion){ // Para cargar la configuración del sistema desde un archivo.
  config = config_create(direccion); // // Crea una estructura de configuración leyendo desde el archivo "memoria.config".
  
  // Verifica si cada propiedad requerida existe en el archivo de configuración y muestra un mensaje de advertencia si falta alguna.
  if (!config_has_property(config, "PUERTO_ESCUCHA")) printf("El valor PUERTO no existe en la config\n");
  if (!config_has_property(config, "TAM_MEMORIA")) printf("El valor TAMANIO no existe en la config\n");
  if (!config_has_property(config, "TAM_PAGINA")) printf("El valor TAMANIO_PAGINA no existe en la config\n");
  if (!config_has_property(config, "PATH_INSTRUCCIONES")) printf("El valor PATH_DUMP_TLB no existe en la config\n");
  if (!config_has_property(config, "RETARDO_RESPUESTA")) printf("El valor RETARDO_RESPUESTA no existe en la config\n");
  
  // Asigna los valores de las propiedades del archivo de configuración a las variables correspondientes en la estructura configuracion.
  
  configuracion.PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
  configuracion.TAMANIO = config_get_int_value(config, "TAM_MEMORIA");
  configuracion.TAMANIO_PAGINAS = config_get_int_value(config, "TAM_PAGINA");
  configuracion.PATH_INSTRUCCIONES = config_get_string_value(config, "PATH_INSTRUCCIONES");
  configuracion.RETARDO_RESPUESTA = config_get_int_value(config, "RETARDO_RESPUESTA");
}
