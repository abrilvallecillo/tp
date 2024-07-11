#include "instrucciones.h"
#include <utils/procesos.h>
#include <stdio.h>
#include "cpu.h"
#include <string.h>
#include <utils/logger_concurrente.h>
#include <commons/string.h>

// Función SET: Asigna al registro el valor pasado como parámetro.
void set(registro_casteado *registro, int valor) { modificarValorRegistroCasteado(registro, valor); }

// Función MOV_IN: Lee el valor de memoria correspondiente a la Dirección Lógica que se encuentra en el Registro Dirección y lo almacena en el Registro Datos.
void mov_in(registro_casteado *registroDatos, registro_casteado *registroDireccion) {
    t_queue * direccionesFisicas = traducirMemoria(leerValorRegistroCasteado(registroDireccion), tamanioRegistroCasteado(registroDatos));
    if (direccionesFisicas == NULL){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    void * valor = malloc(tamanioRegistroCasteado(registroDatos));
    void * resultado = hacerOperacionLecturaMemoriaCPU(direccionesFisicas, valor, pid_actual, registroDatos->es32bits);
    memcpy(registroDatos->registro, resultado, tamanioRegistroCasteado(registroDatos));
    free(resultado);
    queue_destroy(direccionesFisicas);
}

// Función MOV_OUT: Lee el valor del Registro Datos y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica almacenada en el Registro Dirección.
void mov_out(registro_casteado *registroDireccion, registro_casteado *registroDatos) {
    t_queue * direccionesFisicas = traducirMemoria(leerValorRegistroCasteado(registroDireccion), tamanioRegistroCasteado(registroDatos));
    if (direccionesFisicas == NULL){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    void * valor = malloc(tamanioRegistroCasteado(registroDatos));
    memcpy(valor, registroDatos->registro, tamanioRegistroCasteado(registroDatos)); 
    hacerOperacionEscrituraMemoriaCPU(direccionesFisicas, valor, pid_actual, registroDatos->es32bits);
    queue_destroy(direccionesFisicas);
    free(valor);
}

// Función SUM: Suma al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sum(registro_casteado *registroDestino, registro_casteado *registroOrigen) {
    int valor_nuevo = leerValorRegistroCasteado(registroDestino) + leerValorRegistroCasteado(registroOrigen); 
    modificarValorRegistroCasteado(registroDestino, valor_nuevo);
}

// Función SUB: Resta al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sub(registro_casteado *registroOrigen, registro_casteado *registroDestino) { 
    int valor_nuevo = leerValorRegistroCasteado(registroDestino) - leerValorRegistroCasteado(registroOrigen); 
    modificarValorRegistroCasteado(registroDestino, valor_nuevo); 
}

// Función JNZ: Si el valor del registro es distinto de cero, actualiza el program counter al número de instrucción pasada por parámetro.
void jnz(registro_casteado *registro, int nroInstruccion) {
        if (leerValorRegistroCasteado(registro)) {
            registrosPropios.PC = (uint32_t) nroInstruccion;
            incrementaPC = false;
         }
}
// Función RESIZE: Solicitará a la Memoria ajustar el tamaño del proceso al tamaño pasado por parámetro. En caso de que la respuesta de la memoria sea Out of Memory, se deberá devolver el contexto de ejecución al Kernel informando de esta situación.
void resize(int tamanio) {
    peticion_resize * peticion = malloc(sizeof(peticion_resize));
    peticion->pid = pid_actual;
    peticion->tamanio = tamanio;
    
    t_buffer * buffer = serializarPeticionResize(peticion);
    enviarBufferPorPaquete(buffer, P_RESIZE, conexion_memoria);
    free(peticion);
    t_paquete * respuesta = recibirPaqueteGeneral(conexion_memoria);
    
    if(respuesta->codigoOperacion == OUT_OF_MEMORY){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
    }

    eliminar_paquete(respuesta);
}

// Función COPY_STRING: Toma del string apuntado por el registro SI y copia la cantidad de bytes indicadas en el parámetro tamaño a la posición de memoria apuntada por el registro DI. 
void copy_string(int tamanio) {
    t_queue * direccionesFisicas = traducirMemoria(registrosPropios.SI, tamanio);

    if (direccionesFisicas == NULL){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    void * valor = malloc(tamanio);
    char * string = (char *) hacerOperacionLecturaMemoriaCPU(direccionesFisicas, valor, pid_actual, 2);
    queue_destroy(direccionesFisicas);
    direccionesFisicas = traducirMemoria(registrosPropios.DI, tamanio);

    if (direccionesFisicas == NULL){
        free(valor);
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    hacerOperacionEscrituraMemoriaCPU(direccionesFisicas, string, pid_actual, 2);
    queue_destroy(direccionesFisicas);
    free(valor);
}

// Función WAIT (Recurso): Esta instrucción solicita al Kernel que se asigne una instancia del recurso indicado por parámetro.
void wait(char * recurso) {
    queue_push(cola_parametros, recurso);
    modificarEnteroSincronizado(estado_interrupcion, P_WAIT);
}

// Función SIGNAL (Recurso): Esta instrucción solicita al Kernel que se libere una instancia del recurso indicado por parámetro.
void signal(char * recurso) {
    queue_push(cola_parametros, recurso);
    modificarEnteroSincronizado(estado_interrupcion, P_SIGNAL);
}

// Función IO_GEN_SLEEP
void io_gen_sleep(char* interfaz, int  *unidadesDeTrabajo){
    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, unidadesDeTrabajo);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_GEN_SLEEP);
}

// Función IO_STDIN_READ: Esta instrucción solicita al Kernel que mediante la interfaz ingresada se lea desde el STDIN (Teclado) un valor cuyo tamaño está delimitado por el valor del Registro Tamaño y el mismo se guarde a partir de la Dirección Lógica almacenada en el Registro Dirección.
void io_stdin_read(char* interfaz, registro_casteado *registroDireccion, registro_casteado *registroTamanio) {
    t_queue * direccionesFisicas = traducirMemoria(leerValorRegistroCasteado(registroDireccion), leerValorRegistroCasteado(registroTamanio));
    
    if (direccionesFisicas == NULL){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, direccionesFisicas);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_STDIN_READ);
}

// Función IO_STDOUT_WRITE: Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se lea desde la posición de memoria indicada por la Dirección Lógica almacenada en el Registro Dirección, un tamaño indicado por el Registro Tamaño y se imprima por pantalla.
void io_stdout_write(char* interfaz, registro_casteado *registroDireccion, registro_casteado *registroTamanio) {
    t_queue * direccionesFisicas = traducirMemoria(leerValorRegistroCasteado(registroDireccion), leerValorRegistroCasteado(registroTamanio));
   
   if (direccionesFisicas == NULL){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, direccionesFisicas);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_STDOUT_WRITE);
}

// FunciónFunciónIO_FS_CREATE: Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se cree un archivo en el FS montado en dicha interfaz.
void io_fs_create(char* interfaz, char* nombreArchivo) {
    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, nombreArchivo);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_FS_CREATE);
}

// Función IO_FS_DELETE: Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se elimine un archivo en el FS montado en dicha interfaz
void io_fs_delete(char* interfaz, char* nombreArchivo) {
    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, nombreArchivo);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_FS_DELETE);
}

// Función IO_FS_TRUNCATE: Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se modifique el tamaño del archivo en el FS montado en dicha interfaz, actualizando al valor que se encuentra en el registro indicado por Registro Tamaño.
void io_fs_truncate(char* interfaz, char* nombreArchiv, registro_casteado *registroTamanio) {
    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, nombreArchiv);
    int * tamanioNuevo = malloc(sizeof(int));
    *tamanioNuevo = leerValorRegistroCasteado(registroTamanio);
    queue_push(cola_parametros, tamanioNuevo);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_FS_TRUNCATE);
}

// Función IO_FS_WRITE: Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se lea desde Memoria la cantidad de bytes indicadas por el Registro Tamaño a partir de la dirección lógica que se encuentra en el Registro Dirección y se escriban en el archivo a partir del valor del Registro Puntero Archivo.
void io_fs_write(char* interfaz, char* nombreArchiv, registro_casteado *registroDireccion, registro_casteado *registroTamanio, registro_casteado *registropunteroArchivo) {
    t_queue * direccionesFisicas = traducirMemoria(leerValorRegistroCasteado(registroDireccion), leerValorRegistroCasteado(registroTamanio));
    
    if (direccionesFisicas == NULL){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, nombreArchiv);
    int * punteroArchivo = malloc(sizeof(int));
    *punteroArchivo = leerValorRegistroCasteado(registropunteroArchivo);
    queue_push(cola_parametros, punteroArchivo);
    queue_push(cola_parametros, direccionesFisicas);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_FS_WRITE);
}

// Función IO_FS_READ: Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se lea desde el archivo a partir del valor del Registro Puntero Archivo la cantidad de bytes indicada por Registro Tamaño y se escriban en la Memoria a partir de la dirección lógica indicada en el Registro Dirección.
void io_fs_read(char* interfaz, char* nombreArchiv, registro_casteado *registroDireccion, registro_casteado *registroTamanio, registro_casteado *registropunteroArchivo) {
    t_queue * direccionesFisicas = traducirMemoria(leerValorRegistroCasteado(registroDireccion), leerValorRegistroCasteado(registroTamanio));
    
    if (direccionesFisicas == NULL){
        modificarEnteroSincronizado(estado_interrupcion, OUT_OF_MEMORY);
        return;
    }

    queue_push(cola_parametros, interfaz);
    queue_push(cola_parametros, nombreArchiv);
    int * punteroArchivo = malloc(sizeof(int));
    *punteroArchivo = leerValorRegistroCasteado(registropunteroArchivo);
    queue_push(cola_parametros, punteroArchivo);
    queue_push(cola_parametros, direccionesFisicas);
    modificarEnteroSincronizado(estado_interrupcion, P_IO_FS_READ);
}

// Función EXIT: Esta instrucción representa la syscall de finalización del proceso. Se deberá devolver el Contexto de Ejecución actualizado al Kernel para su finalización.
void exitProgram() { modificarEnteroSincronizado(estado_interrupcion, P_EXIT);}

void hacerOperacionEscrituraMemoriaCPU(t_queue * direccionesFisicas, void * valor, uint32_t pid, int reg8_reg32_cpystr) {
    uint32_t offset = 0;
    
    while(!queue_is_empty(direccionesFisicas)){
        t_direccionMemoria * direccion = (t_direccionMemoria *) queue_pop(direccionesFisicas);
        t_operacionMemEscribirUsuario * operacion = crearOperacionMemEscribirUsuario(direccion, pid, valor + offset);
        
        if (reg8_reg32_cpystr == 0) {
            uint8_t * valorI = malloc(sizeof(uint8_t));
            *valorI = (uint8_t) 0;
            memcpy(valorI, operacion->valor, operacion->direccion->tamanioEnvio);
            char * mensaje = string_from_format("PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d", pid, direccion->direccionFisica, *valorI);
            logInfoSincronizado(mensaje);
            free(mensaje);
            free(valorI);
        
        } else if(reg8_reg32_cpystr == 1) {
            uint32_t * valorI = malloc(sizeof(uint32_t));
            *valorI = (uint32_t) 0;
            memcpy(valorI, operacion->valor, operacion->direccion->tamanioEnvio);
            char * mensaje = string_from_format("PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d", pid, direccion->direccionFisica, *valorI);
            logInfoSincronizado(mensaje);
            free(mensaje);
            free(valorI);
       
        } else {
            int tamanio_a_reservar = operacion->direccion->tamanioEnvio;
            tamanio_a_reservar++;
            char * string = malloc(tamanio_a_reservar);
            int offset = 0;

            while(tamanio_a_reservar > 0) {
                tamanio_a_reservar -= 1;
                char auxiliar = '\0';
                memcpy(string + offset, &auxiliar, sizeof(char));
                offset += sizeof(char);
            }

            memcpy(string, operacion->valor, operacion->direccion->tamanioEnvio);
            char * mensaje = string_from_format("PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", pid, direccion->direccionFisica, string);
            logInfoSincronizado(mensaje);
            free(mensaje);
            free(string);
        }

        t_buffer * buffer = serializarOperacionMemEscribirUsuario(operacion);
        enviarBufferPorPaquete(buffer, ESCRIBIR_MEMORIA, conexion_memoria);
        t_paquete * paquete = recibirPaqueteGeneral(conexion_memoria);
        eliminar_paquete(paquete);
        offset += direccion->tamanioEnvio;
        free(direccion);
        free(operacion->valor);
        free(operacion);
    }
}

void * hacerOperacionLecturaMemoriaCPU(t_queue * direccionesFisicas, void * valor, uint32_t pid, int reg8_reg32_cpystr){
    uint32_t offset = 0;
   
    while(!queue_is_empty(direccionesFisicas)){
        t_direccionMemoria * direccion = (t_direccionMemoria *) queue_pop(direccionesFisicas);
        t_operacionMemLeerUsuario * operacion = crearOperacionMemLeerUsuario(direccion, pid);
        t_buffer * buffer = serializarOperacionMemLeerUsuario(operacion);
        enviarBufferPorPaquete(buffer, LEER_MEMORIA, conexion_memoria);
        t_paquete * paquete_repuesta = recibirPaqueteGeneral(conexion_memoria);
        memcpy(valor + offset, paquete_repuesta->buffer->stream, direccion->tamanioEnvio);
        
        if (reg8_reg32_cpystr == 0) {
            uint8_t * valorI = malloc(sizeof(uint8_t));
            *valorI = (uint8_t) 0;
            memcpy(valorI, paquete_repuesta->buffer->stream, operacion->direccion->tamanioEnvio);
            char * mensaje = string_from_format("PID: %d - Acción: LEER - Dirección Física: %d - Valor: %d", pid, direccion->direccionFisica, *valorI);
            logInfoSincronizado(mensaje);
            free(mensaje);
            free(valorI);
        
        } else if(reg8_reg32_cpystr == 1) {
            uint32_t * valorI = malloc(sizeof(uint32_t));
            *valorI = (uint32_t) 0;
            memcpy(valorI, paquete_repuesta->buffer->stream, operacion->direccion->tamanioEnvio);
            char * mensaje = string_from_format("PID: %d - Acción: LEER - Dirección Física: %d - Valor: %d", pid, direccion->direccionFisica, *valorI);
            logInfoSincronizado(mensaje);
            free(mensaje);
            free(valorI);
       
        } else {
            int tamanio_a_reservar = operacion->direccion->tamanioEnvio;
            tamanio_a_reservar++;
            char * string = malloc(tamanio_a_reservar);
            int offset = 0;
            
            while(tamanio_a_reservar > 0) {
                tamanio_a_reservar -= 1;
                char auxiliar = '\0';
                memcpy(string + offset, &auxiliar, sizeof(char));
                offset += sizeof(char);
            }
            memcpy(string, paquete_repuesta->buffer->stream, operacion->direccion->tamanioEnvio);
            char * mensaje = string_from_format("PID: %d - Acción: LEER - Dirección Física: %d - Valor: %s", pid, direccion->direccionFisica, string);
            logInfoSincronizado(mensaje);
            free(mensaje);
            free(string);
        }
       
        offset += direccion->tamanioEnvio;
        free(direccion);
        free(operacion);
        eliminar_paquete(paquete_repuesta);
  
    } return valor;
}
