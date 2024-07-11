#include "consola.h"
#include <readline/readline.h>
#include <commons/string.h>
#include "kernel.h"
#include <stdlib.h>
#include "configuracion.h"
#include <utils/logger_concurrente.h>

peticion_memoria * CrearPeticionCrearProceso(char * ruta_codigo);
bool detenerPlanificacion = false;
void * consola(void * parametro_nulo) {
    char * leer_cadena = readline("> ");

    while(1) {
        leer_linea(leer_cadena);
        free(leer_cadena);
        leer_cadena = readline("> ");
    }
    free(leer_cadena);
    return NULL;
}

void ejecutarComando(char *comando, char *parametro) {
    if (!strcmp(comando, "EJECUTAR_SCRIPT")){
        FILE *script_comandos = fopen(parametro, "r");
        char *lectura_linea = malloc(256*sizeof(char));
        if (script_comandos == NULL){fprintf(stderr, "El archivo esta vacio"); abort();}
        while(fgets(lectura_linea, 256, script_comandos)){
            *(lectura_linea + strcspn(lectura_linea, "\n")) = ' ';
            leer_linea(lectura_linea);
        }
        free(lectura_linea);
        fclose(script_comandos);
    }
    if (!strcmp(comando, "INICIAR_PROCESO")){
        peticion_memoria * crear_proceso = CrearPeticionCrearProceso(parametro);
        agregarACola(cola_memoria, crear_proceso);
        pid_nuevo_proceso++;
        sem_post(hay_peticiones_memoria);
    }
    if (!strcmp(comando, "MULTIPROGRAMACION")){
        int valor = (int) strtol(parametro, NULL, 10);
        modificarEnteroSincronizado(grado_multiprogramacion, valor);
        modificarEnteroSincronizado(diferencia_cambio_mp, valor - configuracion.GRADO_MULTIPROGRAMACION);
        
        int delta_MP = leerEnteroSincronizado(diferencia_cambio_mp); 
        while(delta_MP > 0){
            sem_post(grado_multiprogramacion_habilita);
            disminuirEnteroSincronizado(diferencia_cambio_mp);
            delta_MP = leerEnteroSincronizado(diferencia_cambio_mp); 
        }
    }
    if (!strcmp(comando, "DETENER_PLANIFICACION")) {
        if(!detenerPlanificacion) {
            detenerPlanificacion = true;
            sem_wait(hay_que_parar_planificacion);
        }
    }
    if (!strcmp(comando, "INICIAR_PLANIFICACION")) {
        if(detenerPlanificacion) {
            sem_post(hay_que_parar_planificacion);
            detenerPlanificacion = false;
        }
    } 
    if (!strcmp(comando, "FINALIZAR_PROCESO")) {
        int valor = (int) strtol(parametro, NULL, 10);
        int estado_actual = obtenerEstadoPorPID(valor);  
        if(estado_actual != -1) {
            modificarEnteroSincronizado(pid_fin_usuario, valor);
            if(estado_actual == EXEC) {
                sem_post(solicitaron_finalizar_proceso);
            } else {
                if(!detenerPlanificacion) {
                    sem_wait(hay_que_parar_planificacion);
                }
                buscarYTratarDeEliminarEn(estado_actual, valor);
                if(!detenerPlanificacion) {
                    sem_post(hay_que_parar_planificacion);
                }
            }
        }
        
    }
    if (!strcmp(comando, "PROCESO_ESTADO")) {
        if(!detenerPlanificacion) {
            sem_wait(hay_que_parar_planificacion);
        }
        listarPIDsyEstadosActuales();
        if(!detenerPlanificacion) {
            sem_post(hay_que_parar_planificacion);
        }
    }
}

void leer_linea(char* leer_cadena){
    char** array_cadena = string_split(leer_cadena, " ");
    ejecutarComando(array_cadena[0], array_cadena[1]);
     string_array_destroy(array_cadena);
}



peticion_memoria * CrearPeticionCrearProceso(char * ruta_codigo) {
    peticion_memoria * crear_proceso = crearPeticionMemoria(CREAR_PROCESO);
    int * pid = malloc(sizeof(int));
    int * quantum = malloc(sizeof(int));
    int * longitud_ruta_codigo = malloc(sizeof(int));
    *pid = pid_nuevo_proceso;
    *quantum = configuracion.QUANTUM;
    *longitud_ruta_codigo = strlen(ruta_codigo) + 1;
    queue_push(crear_proceso->cola_parametros, pid);
    queue_push(crear_proceso->cola_parametros, quantum);
    queue_push(crear_proceso->cola_parametros, longitud_ruta_codigo);
    char * ruta_dupli = string_duplicate(ruta_codigo);
    queue_push(crear_proceso->cola_parametros, ruta_dupli);
    return crear_proceso;
}