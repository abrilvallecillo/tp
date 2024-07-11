#include "configuracion.h" 

t_config* config;
t_cfg configuracion;

void obtener_config(char * direccion_config){ // Para cargar la configuración del sistema desde un archivo.
  config = config_create(direccion_config); // // Crea una estructura de configuración leyendo desde el archivo "cpu.config".
  
  // Verifica si cada propiedad requerida existe en el archivo de configuración y muestra un mensaje de advertencia si falta alguna.
  if (!config_has_property(config, "PUERTO_ESCUCHA")) printf("El valor PUERTO_ESCUCHA no existe en la config\n");
 
  if (!config_has_property(config, "IP_MEMORIA")) printf("El valor IP_MEMORIA no existe en la config\n");
  if (!config_has_property(config, "PUERTO_MEMORIA")) printf("El valor PUERTO_MEMORIA no existe en la config\n");

  if (!config_has_property(config, "IP_CPU")) printf("El valor IP_CPU no existe en la config\n");
  if (!config_has_property(config, "PUERTO_CPU_DISPATCH")) printf("El valor PUERTO_CPU_DISPATCH no existe en la config\n");
  if (!config_has_property(config, "PUERTO_CPU_INTERRUPT")) printf("El valor PUERTO_CPU_INTERRUPT no existe en la config\n");
  
  if (!config_has_property(config, "ALGORITMO_PLANIFICACION")) printf("El valor ALGORITMO_PLANIFICACION no existe en la config\n");
  if (!config_has_property(config, "QUANTUM")) printf("El valor QUANTUM no existe en la config\n");

  if (!config_has_property(config, "RECURSOS")) printf("El valor RECURSOS no existe en la config\n");
  if (!config_has_property(config, "INSTANCIAS_RECURSOS")) printf("El valor INSTANCIAS_RECURSOS no existe en la config\n");
  
  if (!config_has_property(config, "GRADO_MULTIPROGRAMACION")) printf("El valor GRADO_MULTIPROGRAMACION no existe en la config\n");
  
  // Asigna los valores de las propiedades del archivo de configuración a las variables correspondientes en la estructura configuracion.
  
  configuracion.PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
  
  configuracion.IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
  configuracion.PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
  
  configuracion.IP_CPU = config_get_string_value(config, "IP_CPU");
  configuracion.PUERTO_CPU_DISPATCH = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
  configuracion.PUERTO_CPU_INTERRUPT = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");

  configuracion.ALGORITMO_PLANIFICACION = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
  configuracion.QUANTUM = config_get_int_value(config, "QUANTUM");

  configuracion.RECURSOS = config_get_array_value(config, "RECURSOS");
  configuracion.INSTANCIAS_RECURSOS = config_get_array_value(config, "INSTANCIAS_RECURSOS");

  configuracion.GRADO_MULTIPROGRAMACION = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
  }