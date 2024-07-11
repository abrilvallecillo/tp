#include "logger_concurrente.h"
#include <stdlib.h>
t_log * logger;
pthread_mutex_t * mutex_logger;


void inicializarLogger(char * nombre, char * direccion) {
    logger = log_create(direccion, nombre, true, LOG_LEVEL_INFO);
    if(!logger) {
        fprintf(stderr, "Error al crear el logger");
        abort();
    }
    mutex_logger = malloc(sizeof(pthread_mutex_t));
    int mutex_mal_creado = pthread_mutex_init(mutex_logger, NULL);
    if(mutex_mal_creado) {
        fprintf(stderr, "Error al crear el mutex del logger");
        abort();
    }
}


void logInfoSincronizado(const char * mensaje) {
    log_info(logger, mensaje);
}

void destruirLogger(void) {
    log_destroy(logger);
    pthread_mutex_destroy(mutex_logger);
}