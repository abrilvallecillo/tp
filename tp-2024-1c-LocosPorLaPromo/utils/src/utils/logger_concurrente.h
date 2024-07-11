#ifndef LOGGER_CONCURRENTE_H_
#define LOGGER_CONCURRENTE_H_

#include <commons/log.h>
#include <pthread.h>
extern pthread_mutex_t * mutex_logger;
extern t_log * logger;

void inicializarLogger(char * nombre, char * direccion);
void destruirLogger(void);
void logInfoSincronizado(const char * mensaje);

#endif