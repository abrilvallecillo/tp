#include "configuracion.h" 

t_config* config; 
t_cfg configuracion;

void obtener_config(char * direccion){ // Para cargar la configuración del sistema desde un archivo.
  config = config_create(direccion); // // Crea una estructura de configuración leyendo desde el archivo "cpu.config".
  
  // Verifica si cada propiedad requerida existe en el archivo de configuración y muestra un mensaje de advertencia si falta alguna.
  if (!config_has_property(config, "TIPO_INTERFAZ")) printf("El valor TIPO_INTERFAZ no existe en la config\n");
  if (!config_has_property(config, "TIEMPO_UNIDAD_TRABAJO")) printf("El valor TIEMPO_UNIDAD_TRABAJO no existe en la config\n");

  if (!config_has_property(config, "IP_KERNEL")) printf("El valor IP_KERNEL no existe en la config\n");
  if (!config_has_property(config, "PUERTO_KERNEL")) printf("El valor PUERTO_KERNEL no existe en la config\n");

  if (!config_has_property(config, "IP_MEMORIA")) printf("El valor IP_MEMORIA no existe en la config\n");
  if (!config_has_property(config, "PUERTO_MEMORIA")) printf("El valor PUERTO_MEMORIA no existe en la config\n");
  
  if (!config_has_property(config, "PATH_BASE_DIALFS")) printf("El valor PATH_BASE_DIALFS no existe en la config\n");
  if (!config_has_property(config, "BLOCK_SIZE")) printf("El valor BLOCK_SIZE no existe en la config\n");
  if (!config_has_property(config, "BLOCK_COUNT")) printf("El valor BLOCK_COUNT no existe en la config\n");
  if (!config_has_property(config, "RETRASO_COMPACTACION")) printf("El valor RETRASO_COMPACTACION no existe en la config\n");
  
  // Asigna los valores de las propiedades del archivo de configuración a las variables correspondientes en la estructura configuracion.
  
  configuracion.TIPO_INTERFAZ = config_get_string_value(config, "TIPO_INTERFAZ");
  configuracion.TIEMPO_UNIDAD_TRABAJO = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");

  configuracion.IP_KERNEL = config_get_string_value(config, "IP_KERNEL");
  configuracion.PUERTO_KERNEL = config_get_string_value(config, "PUERTO_KERNEL");

  configuracion.IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
  configuracion.PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");

  configuracion.PATH_BASE_DIALFS = config_get_string_value(config, "PATH_BASE_DIALFS");
  configuracion.BLOCK_SIZE = config_get_int_value(config, "BLOCK_SIZE");
  configuracion.BLOCK_COUNT = config_get_int_value(config, "BLOCK_COUNT");
  configuracion.RETRASO_COMPACTACION = config_get_int_value(config, "RETRASO_COMPACTACION");
}