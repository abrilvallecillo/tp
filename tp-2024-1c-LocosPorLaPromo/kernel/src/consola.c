#include "consola.h"
#include <readline/readline.h>
#include <commons/string.h>
#include "kernel.h"
#include <stdlib.h>
#include "configuracion.h"
#include <utils/logger_concurrente.h>

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

    // -------------------------- Ejecutar Script de Operaciones --------------------------
    // Se encargará de abrir un archivo de comandos que se encontrará en la máquina donde corra el Kernel, el mismo contendrá una secuencia de comandos y se los deberá ejecutar uno a uno hasta finalizar el archivo.

    if (!strcmp(comando, "EJECUTAR_SCRIPT")){
        FILE *script_comandos = fopen(parametro, "r");
        char *lectura_linea = malloc(256*sizeof(char));

        if (script_comandos == NULL){
            fprintf(stderr, "El archivo esta vacio"); 
            abort();
        }

        while(fgets(lectura_linea, 256, script_comandos)){
            *(lectura_linea + strcspn(lectura_linea, "\n")) = ' '; // Convierte string en long
            leer_linea(lectura_linea);
        }

        free(lectura_linea);
        fclose(script_comandos);
    }

    // -------------------------- Iniciar proceso --------------------------
    // Se encargará de iniciar un nuevo proceso cuyas instrucciones a ejecutar se encontrarán en el archivo <path> dentro del file system de la máquina donde corra la Memoria. 
    // Este mensaje se encargará de la creación del proceso (PCB) en estado NEW.

    if (!strcmp(comando, "INICIAR_PROCESO")){
        peticion_memoria * crear_proceso = CrearPeticionCrearProceso(parametro);
        agregarACola(cola_memoria, crear_proceso);
        pid_nuevo_proceso++;
        sem_post(hay_peticiones_memoria);
    }

    // -------------------------- Finalizar Proceso --------------------------
    // Se encargará de finalizar un proceso que se encuentre dentro del sistema. 
    // Este mensaje se encargará de realizar las mismas operaciones como si el proceso llegara a EXIT por sus caminos habituales (deberá liberar recursos y memoria).

    if (!strcmp(comando, "FINALIZAR_PROCESO")) {
        int valor = (int) strtol(parametro, NULL, 10);
        int estado_actual = obtenerEstadoPorPID(valor);  

        if(estado_actual != -1) {
            modificarEnteroSincronizado(pid_fin_usuario, valor);

            if(estado_actual == EXEC) {
                sem_post(solicitaron_finalizar_proceso);

            } else {

            // detenerPlanificacion = true
                if(!detenerPlanificacion) {
            sem_wait(hay_que_parar_planificacion);
                }

                buscarYTratarDeEliminarEn(estado_actual, valor);

                if(!detenerPlanificacion) {
            sem_post(hay_que_parar_planificacion);

                }
            // detenerPlanificacion = false
            // buscarYTratarDeEliminarEn(estado_actual, valor);            
            
            }
        }
    }

    // -------------------------- Iniciar Planificacion --------------------------
    // Este mensaje se encargará de retomar (en caso que se encuentre pausada) la planificación de corto y largo plazo. 
    // En caso que la planificación no se encuentre pausada, se debe ignorar el mensaje.

    if (!strcmp(comando, "INICIAR_PLANIFICACION")) {
        if(detenerPlanificacion) {
            sem_post(hay_que_parar_planificacion);
            detenerPlanificacion = false;
        }
    }

    // -------------------------- Detener Planificacion --------------------------
    // Este mensaje se encargará de pausar la planificación de corto y largo plazo. 
    // El proceso que se encuentra en ejecución NO es desalojado, pero una vez que salga de EXEC se va a pausar el manejo de su motivo de desalojo. 
    // De la misma forma, los procesos bloqueados van a pausar su transición a la cola de Ready.

    if (!strcmp(comando, "DETENER_PLANIFICACION")) {
        if(!detenerPlanificacion) {
            detenerPlanificacion = true;
            sem_wait(hay_que_parar_planificacion);
        }
    }

    // -------------------------- Listar Estados de Procesos --------------------------
    // Se encargará de mostrar por consola el listado de los estados con los procesos que se encuentran dentro de cada uno de ellos.
    
    if (!strcmp(comando, "PROCESO_ESTADO")) {
        if(!detenerPlanificacion) {
            sem_wait(hay_que_parar_planificacion);
        }
        listarPIDsyEstadosActuales();
        if(!detenerPlanificacion) {
            sem_post(hay_que_parar_planificacion);
        }
    }

    // -------------------------- Modificar Nivel de Multiprogramacion --------------------------
    // Modificar el grado de multiprogramación del módulo: 
    // Se cambiará el grado de multiprogramación del sistema reemplazandolo por el valor indicado.

    if (!strcmp(comando, "MULTIPROGRAMACION")){
        int valor = (int) strtol(parametro, NULL, 10);
        modificarEnteroSincronizado(grado_multiprogramacion, valor);
        modificarEnteroSincronizado(diferencia_cambio_mp, valor - configuracion.GRADO_MULTIPROGRAMACION); // variacion del grado de multiprogramacion
        
        int delta_MP = leerEnteroSincronizado(diferencia_cambio_mp); 
        
        while(delta_MP > 0){ // Auemnto el grado de multiporgramacion
            sem_post(grado_multiprogramacion_habilita);
            disminuirEnteroSincronizado(diferencia_cambio_mp);
            delta_MP = leerEnteroSincronizado(diferencia_cambio_mp); 
        }
    }
}