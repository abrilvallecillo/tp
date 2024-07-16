#ifndef CONSOLA_H_
#define CONSOLA_H_

peticion_memoria * CrearPeticionCrearProceso(char * ruta_codigo);
bool detenerPlanificacion = false;
void * consola(void * parametro_nulo);
void ejecutarComando(char *comando, char *parametro);
void leer_linea(char* leer_cadena);

#endif