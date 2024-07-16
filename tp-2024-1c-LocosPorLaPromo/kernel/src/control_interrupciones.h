#ifndef CONTROL_INTERRUPCIONES_H_
#define CONTROL_INTERRUPCIONES_H_
#include "configuracion.h"
pthread_t manejo_interrupciones_quantum;

void enviarInterrupcionPorQuantum(int conexion_cpu_interrupt);
void enviarInterrupcionACPUConsola(int conexion_cpu_interrupt);
void * manejarInterrupcionesQuantum(void * conexion_interrupt);

void *interrupcionDeProcesos(void * sin_parametro);
#endif