#include "ciclo_instruccion.h"
#include "instrucciones.h"
#include <utils/procesos.h>
#include <utils/conexiones.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include "cpu.h"
#include <string.h>
#include <utils/logger_concurrente.h>

// -------------------------- Casteo de Instrucciones y Registros --------------------------

int casteoStringInstruccion(char* instruccion){
    if(!strcmp("SET", instruccion)){ return SET; }
    if(!strcmp("MOV_IN", instruccion)){ return MOV_IN; }
    if(!strcmp("MOV_OUT", instruccion)){ return MOV_OUT; }
    if(!strcmp("SUM", instruccion)){ return SUM; }
    if(!strcmp("SUB", instruccion)){ return SUB; }
    if(!strcmp("JNZ", instruccion)){ return JNZ; }
    if(!strcmp("IO_GEN_SLEEP", instruccion)){ return IO_GEN_SLEEP; }
    if(!strcmp("RESIZE", instruccion)){ return RESIZE; }
    if(!strcmp("WAIT", instruccion)){ return WAIT; }
    if(!strcmp("SIGNAL", instruccion)){ return SIGNAL; }
    if(!strcmp("IO_STDIN_READ", instruccion)){ return IO_STDIN_READ; }
    if(!strcmp("IO_STDOUT_WRITE", instruccion)){ return IO_STDOUT_WRITE; }
    if(!strcmp("EXIT", instruccion)){ return EXIT; }
    if(!strcmp("COPY_STRING", instruccion)){ return COPY_STRING; }
    if(!strcmp("IO_FS_CREATE", instruccion)){ return IO_FS_CREATE; }
    if(!strcmp("IO_FS_DELETE", instruccion)){ return IO_FS_DELETE; }
    if(!strcmp("IO_FS_TRUNCATE", instruccion)){ return IO_FS_TRUNCATE; }
    if(!strcmp("IO_FS_WRITE", instruccion)){ return IO_FS_WRITE; }
    if(!strcmp("IO_FS_READ", instruccion)){ return IO_FS_READ; }
    return -1;
}

registro_casteado * casteoRegistro(char * operando_registro){
    if (!strcmp("AX", operando_registro)) { return crearRegistroCasteado(false, &registrosPropios.generales.AX); }
    if (!strcmp("BX", operando_registro)) { return crearRegistroCasteado(false, &registrosPropios.generales.BX); }
    if (!strcmp("CX", operando_registro)) { return crearRegistroCasteado(false, &registrosPropios.generales.CX); }
    if (!strcmp("DX", operando_registro)) { return crearRegistroCasteado(false, &registrosPropios.generales.DX); }
    if (!strcmp("EAX", operando_registro)) { return crearRegistroCasteado(true, &registrosPropios.generales.EAX); }
    if (!strcmp("EBX", operando_registro)) { return crearRegistroCasteado(true, &registrosPropios.generales.EBX); }
    if (!strcmp("ECX", operando_registro)) { return crearRegistroCasteado(true, &registrosPropios.generales.ECX); }
    if (!strcmp("EDX", operando_registro)) { return crearRegistroCasteado(true, &registrosPropios.generales.EDX); }
    if (!strcmp("PC", operando_registro)) { return crearRegistroCasteado(true, &registrosPropios.PC); }
    if (!strcmp("SI", operando_registro)) { return crearRegistroCasteado(true, &registrosPropios.SI); }
    if (!strcmp("DI", operando_registro)) { return crearRegistroCasteado(true, &registrosPropios.DI); }
    return NULL;
}

// -------------------------- Ciclo De Instrucciones --------------------------

void cicloDeInstruccion(uint32_t pid, int volvio_de_interrupcion){
    pid_actual = pid;
    
    do {
        incrementaPC = true;

        char* proxima_instruccion = fetch(pid, registrosPropios.PC);

        instruccion_decodificada * instruccion_a_ejecutar = decode(proxima_instruccion);

        execute(instruccion_a_ejecutar);

        if (incrementaPC){ (registrosPropios.PC) += 1; }
       
        interrupcion_proceso = leerEnteroSincronizado(estado_interrupcion);    

    } while( !esInterrupcionSincronica(interrupcion_proceso) &&  leerEnteroSincronizado(pid_a_finalizar) != pid_actual); 
    
    pthread_mutex_lock(estado_interrupcion->mutex_entero);

        if(estado_interrupcion->entero != INT_FIN_CONSOLA) {
            estado_interrupcion->entero = NO_HAY_INTERRUPCION;
            modificarEnteroSincronizado(pid_a_finalizar, -1);
        } 
        
        if(interrupcion_proceso != P_WAIT && interrupcion_proceso != P_SIGNAL){
            modificarEnteroSincronizado(consumirFinQ, 1);
        }

    pthread_mutex_unlock(estado_interrupcion->mutex_entero);
};

// -------------------------- FETCH --------------------------

char * fetch(uint32_t pid, uint32_t pc){ 
    // Buscar próxima instrucción a ejecutar
    peticion_instruccion * peticion = malloc(sizeof(peticion_instruccion));
    peticion->pid = pid;
    peticion->pc = pc;

    // Cada instrucción deberá ser pedida al módulo Memoria utilizando el Program Counter 
    t_buffer * buffer = serializarPeticionAInstruccion(peticion);
    enviarBufferPorPaquete(buffer, PETICION_INSTRUCCION, conexion_memoria);
    free(peticion);
    
    //Recibir paquete de memoria con proxima instrucción a ejecutar
    t_paquete * paquete_con_instruccion = recibirPaqueteGeneral(conexion_memoria);
    t_instruccion * instruccion = deserializarInstruccion(paquete_con_instruccion->buffer);
    char * cadena_instruccion = string_duplicate(instruccion->string_instruccion);

    eliminar_paquete(paquete_con_instruccion);
    free(instruccion->string_instruccion);
    free(instruccion);

    char * mensaje = string_from_format("PID: %d - FETCH - :  %d ", pid, pc);
    logInfoSincronizado(mensaje);
    free(mensaje);

    return cadena_instruccion;
};

// -------------------------- DECODE --------------------------

// Interpretar qué instrucción es la que se va a ejecutar
// Si la misma requiere de una traducción de dirección lógica a dirección física.

instruccion_decodificada * decode(char * proxima_instruccion){
    instruccion_decodificada * instruccion = malloc(sizeof(instruccion_decodificada));
    instruccion->parametros = queue_create();
    
    *(proxima_instruccion + strcspn(proxima_instruccion, "\n")) = ' ';
    char** array_instruccion; // [operacion, parametro, ....]
   
    if(!string_starts_with(proxima_instruccion, "EXIT")) {
        array_instruccion = string_split(proxima_instruccion, " ");
    } else {
        array_instruccion = string_array_new();
        string_array_push(&array_instruccion, string_duplicate("EXIT"));
        string_array_push(&array_instruccion, string_duplicate(""));
    }
    free(proxima_instruccion);

    instruccion->operacion = casteoStringInstruccion(array_instruccion[0]);
    int cantidad_parametros = string_array_size(array_instruccion)-1-1;
    char * parametros = string_new(); 

    for(int i = 0 ; i < cantidad_parametros; i++){ 
        queue_push(instruccion->parametros, array_instruccion[i+1]); 
        string_append(&parametros, " ");
        string_append(&parametros, array_instruccion[i+1]);
    }
    string_append(&parametros, " ");
    char * operacion = array_instruccion[0];

    char * mensaje = string_from_format("PID: %d - Ejecutando: %s - %s ", pid_actual, operacion, parametros);
    logInfoSincronizado(mensaje);
    free(mensaje);
    free(operacion);
    free(parametros);

    free(string_array_pop(array_instruccion));
    free(array_instruccion);

    return instruccion;
};

// -------------------------- EXECUTE --------------------------

void execute(instruccion_decodificada * instruccion){
    char * auxiliar;
    switch (instruccion->operacion){
        
        case SET:
            auxiliar = queue_pop(instruccion->parametros);
            registro_casteado * registro_operando = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            int valor = atoi(auxiliar);
            free(auxiliar);

            set(registro_operando, valor);

            free(registro_operando);

            break;
            
        case SUM:
            auxiliar = queue_pop(instruccion->parametros);
            registro_casteado * registro_destino = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            registro_casteado * registro_origen = casteoRegistro(auxiliar);
            free(auxiliar);

            sum(registro_destino, registro_origen);

            free(registro_destino);
            free(registro_origen);

            break;
            
        case SUB:
            auxiliar = queue_pop(instruccion->parametros);
            registro_destino = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            registro_origen = casteoRegistro(auxiliar);
            free(auxiliar);

            sub(registro_destino, registro_origen);

            free(registro_destino);
            free(registro_origen);

            break;
            
        case JNZ:
            auxiliar = queue_pop(instruccion->parametros);
            registro_operando = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            valor = atoi(auxiliar);
            free(auxiliar);

            jnz(registro_operando, valor);

            free(registro_operando);

            break;
            
        case IO_GEN_SLEEP:
            char * interfaz = queue_pop(instruccion->parametros);

            auxiliar = queue_pop(instruccion->parametros);
            int * unidades_de_trabajo = malloc(sizeof(int));
            *unidades_de_trabajo = atoi(auxiliar);
            free(auxiliar);

            io_gen_sleep(interfaz, unidades_de_trabajo);

            break;

        case MOV_IN:
            auxiliar = queue_pop(instruccion->parametros);
            registro_casteado * registro_datos = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            registro_casteado * registro_direccion = casteoRegistro(auxiliar);
            free(auxiliar);

            mov_in(registro_datos, registro_direccion);

            free(registro_datos);
            free(registro_direccion);

            break;

        case MOV_OUT:
            auxiliar = queue_pop(instruccion->parametros);
            registro_direccion = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            registro_datos = casteoRegistro(auxiliar);
            free(auxiliar);

            mov_out(registro_direccion, registro_datos);

            free(registro_datos);
            free(registro_direccion);

            break;

        case RESIZE:
            auxiliar = queue_pop(instruccion->parametros);
            valor = atoi(auxiliar);
            free(auxiliar);

            resize(valor);

            break;

        case WAIT:
            char * recurso = queue_pop(instruccion->parametros);

            wait(recurso);

            break;

        case SIGNAL:
            recurso = queue_pop(instruccion->parametros);

            signal(recurso);

            break;

        case IO_STDIN_READ:
            interfaz = queue_pop(instruccion->parametros);
            auxiliar = queue_pop(instruccion->parametros);
            registro_direccion = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            registro_casteado * registro_tamanio = casteoRegistro(auxiliar);
            free(auxiliar);

            io_stdin_read(interfaz, registro_direccion, registro_tamanio);

            free(registro_direccion);
            free(registro_tamanio);

            break;

        case IO_STDOUT_WRITE:
            interfaz = queue_pop(instruccion->parametros);

            auxiliar = queue_pop(instruccion->parametros);
            registro_direccion = casteoRegistro(auxiliar);
            free(auxiliar);

            auxiliar = queue_pop(instruccion->parametros);
            registro_tamanio = casteoRegistro(auxiliar);
            free(auxiliar);

            io_stdout_write(interfaz, registro_direccion, registro_tamanio);

            free(registro_direccion);
            free(registro_tamanio);

            break;

        case COPY_STRING:
            auxiliar = queue_pop(instruccion->parametros);
            valor = atoi(auxiliar);
            free(auxiliar);
            copy_string(valor);

            break;

        case IO_FS_CREATE:
            interfaz = queue_pop(instruccion->parametros);
            char * nombre_archivo = queue_pop(instruccion->parametros);
            io_fs_create(interfaz, nombre_archivo);
            break;

        case IO_FS_DELETE:
            interfaz = queue_pop(instruccion->parametros);
            nombre_archivo = queue_pop(instruccion->parametros);
            io_fs_delete(interfaz, nombre_archivo);
            break;
        
        case IO_FS_TRUNCATE:
            interfaz = queue_pop(instruccion->parametros);
            nombre_archivo = queue_pop(instruccion->parametros);
            auxiliar = queue_pop(instruccion->parametros);
            registro_tamanio = casteoRegistro(auxiliar);
            free(auxiliar);
            io_fs_truncate(interfaz, nombre_archivo, registro_tamanio);
            free(registro_tamanio);
            break;
        
        case IO_FS_WRITE:
            interfaz = queue_pop(instruccion->parametros);
            nombre_archivo = queue_pop(instruccion->parametros);
            auxiliar = queue_pop(instruccion->parametros);
            registro_direccion = casteoRegistro(auxiliar);
            free(auxiliar);
            auxiliar = queue_pop(instruccion->parametros);
            registro_tamanio = casteoRegistro(auxiliar);
            free(auxiliar);
            auxiliar = queue_pop(instruccion->parametros);
            registro_casteado * registro_ptr_archivo = casteoRegistro(auxiliar);
            free(auxiliar);
            io_fs_write(interfaz, nombre_archivo, registro_direccion, registro_tamanio, registro_ptr_archivo);
            free(registro_direccion);
            free(registro_tamanio);
            free(registro_ptr_archivo);
            break;
        
        case IO_FS_READ:
            interfaz = queue_pop(instruccion->parametros);
            nombre_archivo = queue_pop(instruccion->parametros);
            auxiliar = queue_pop(instruccion->parametros);
            registro_direccion = casteoRegistro(auxiliar);
            free(auxiliar);
            auxiliar = queue_pop(instruccion->parametros);
            registro_tamanio = casteoRegistro(auxiliar);
            free(auxiliar);
            auxiliar = queue_pop(instruccion->parametros);
            registro_ptr_archivo = casteoRegistro(auxiliar);
            free(auxiliar);
            io_fs_read(interfaz, nombre_archivo, registro_direccion, registro_tamanio, registro_ptr_archivo);
            free(registro_direccion);
            free(registro_tamanio);
            free(registro_ptr_archivo);
            break;
            
        case EXIT:
            exitProgram();
            break;
        
        default:
            break;
    }
    
    queue_destroy(instruccion->parametros);
    free(instruccion);
}

