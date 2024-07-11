#include "configuracion.h" 

t_config* config;
t_cfg configuracion;

void obtener_config(char * direccion){ // Para cargar la configuración del sistema desde un archivo.
  config = config_create(direccion); // // Crea una estructura de configuración leyendo desde el archivo "cpu.config".
  
  // Verifica si cada propiedad requerida existe en el archivo de configuración y muestra un mensaje de advertencia si falta alguna.
  if (!config_has_property(config, "IP_MEMORIA")) printf("El valor IP_MEMORIA no existe en la config\n");
  if (!config_has_property(config, "PUERTO_MEMORIA")) printf("El valor PUERTO_MEMORIA no existe en la config\n");

  if (!config_has_property(config, "PUERTO_ESCUCHA_DISPATCH")) printf("El valor PUERTO_ESCUCHA_DISPATCH no existe en la config\n");
  if (!config_has_property(config, "PUERTO_ESCUCHA_INTERRUPT")) printf("El valor PUERTO_ESCUCHA_INTERRUPT no existe en la config\n");

  if (!config_has_property(config, "CANTIDAD_ENTRADAS_TLB")) printf("El valor CANTIDAD_ENTRADAS_TLB no existe en la config\n");
  if (!config_has_property(config, "ALGORITMO_TLB")) printf("El valor ALGORITMO_TLB no existe en la config\n");
  
  // Asigna los valores de las propiedades del archivo de configuración a las variables correspondientes en la estructura configuracion.
  
  configuracion.IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
  configuracion.PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");

  configuracion.PUERTO_ESCUCHA_DISPATCH = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
  configuracion.PUERTO_ESCUCHA_INTERRUPT = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

  configuracion.CANTIDAD_ENTRADAS_TLB = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
  configuracion.ALGORITMO_TLB = config_get_string_value(config, "ALGORITMO_TLB");

}