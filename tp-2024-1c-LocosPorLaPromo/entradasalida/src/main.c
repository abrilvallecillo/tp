#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <utils/hilos.h>
#include <utils/conexiones.h>
#include <commons/log.h>
#include <commons/config.h> 
#include <utils/procesos.h>
#include <utils/interfaces.h>
#include "configuracion.h"
#include "iogen.h"
#include <commons/string.h>
#include <string.h>
#include <utils/logger_concurrente.h>
#include <commons/string.h> 


int main(int argc, char* argv[]) {
    archivos_dial_fs = list_create();

    inicializarLogger("entradasalida.log", "Log de IO");
    // Enviando un handshake para establecer la comunicación con él.
    char * direccion_config;
    if(argc < 3) {
        direccion_config = string_duplicate("entradasalida.config");
    } else {
        direccion_config = string_duplicate(argv[2]);
    }
    obtener_config(direccion_config);

    int conexion_kernel = crearConexionCliente(configuracion.IP_KERNEL, configuracion.PUERTO_KERNEL);
    int handshake_kernel = enviarHandshake(conexion_kernel, ENTRADASALIDA);
    if(handshake_kernel == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con la memoria");
        abort();
    }
    // Enviando un handshake para establecer la comunicación con él.
    int conexion_memoria = crearConexionCliente(configuracion.IP_MEMORIA, configuracion.PUERTO_MEMORIA);
    int handshake_memoria = enviarHandshake(conexion_memoria, ENTRADASALIDA);
    if(handshake_memoria == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con la memoria");
        abort();
    }
    
    
    info_interfaz* tipo_interfaz = crearTipoInterfaz(argv[1], configuracion.TIPO_INTERFAZ);
    t_buffer* buffer_interfaz = serializarInformacionInterfaz(tipo_interfaz);
    enviarBufferProcesoConMotivo(buffer_interfaz, INFO_INTERFAZ, conexion_kernel);

    if(tipo_interfaz->tipo == DIALFS) {
        iniciarDialFS();
    }

    while(1){
        t_paquete* paquete_kernel = recibirPaqueteGeneral(conexion_kernel);
        if(!paquete_kernel)
            break;
        switch(paquete_kernel->codigoOperacion){
            case P_IO_GEN_SLEEP:
                operacionIOGENSLEEP * operacion = deserializarOperacionIOGENSLEEP(paquete_kernel->buffer);
                char * mensaje = string_from_format ("PID: %d - Operacion: %s", operacion->pid, "IO_GEN_SLEEP");
                logInfoSincronizado(mensaje);
                free(mensaje);
                usleep(operacion->unidades_de_trabajo * configuracion.TIEMPO_UNIDAD_TRABAJO * 1000);
                t_buffer* buffer = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer, P_IO_GEN_SLEEP, conexion_kernel);
                free(operacion);
                break;
            case P_IO_STDOUT_WRITE:
                // Deserializar dirección física
                operacionSTDInOut * operacionOut = deserializarOperacionSTDInOut(paquete_kernel->buffer);
                mensaje = string_from_format ("PID: %d - Operacion: %s", operacionOut->pid, "IO_STDOUT_WRITE");
                logInfoSincronizado(mensaje);
                free(mensaje);
                // Recibir valor desde la memoria
                int tamanio_a_reservar = tamanioDireccionesFisicas(operacionOut->direccionesFisicas, operacionOut->cantidad_direccionesFisicas); 
                tamanio_a_reservar++;
                void * valor = malloc(tamanio_a_reservar);
                int offset = 0;
                while(tamanio_a_reservar > 0) {
                    tamanio_a_reservar -= 1;
                    char auxiliar = '\0';
                    memcpy(valor + offset, &auxiliar, sizeof(char));
                    offset += sizeof(char);
                }
                char* valor_leido = hacerOperacionLecturaMemoria(operacionOut->direccionesFisicas, valor, operacionOut->pid, conexion_memoria);

                // Mostrar el valor por pantalla
                printf("Valor leído desde memoria: %s\n", valor_leido);

                // Enviar confirmación al Kernel
                t_buffer* buffer_confirmacion = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer_confirmacion, P_IO_STDOUT_WRITE, conexion_kernel);
                free(valor_leido);
                queue_destroy(operacionOut->direccionesFisicas);
                free(operacionOut);
                break;
            case P_IO_STDIN_READ:
                operacionSTDInOut * operacionIn = deserializarOperacionSTDInOut(paquete_kernel->buffer);
                mensaje = string_from_format ("PID: %d - Operacion: %s", operacionIn->pid, "IO_STDIN_READ");
                logInfoSincronizado(mensaje);
                free(mensaje);
                // Leer texto del usuario
                char string[256];
                printf("Ingrese texto: ");
                fgets(string, 256, stdin);
                string[strcspn(string, "\n")] = '\0';  // Remove newline character

                // Deserializar dirección física
                
                
                // Enviar texto a la memoria
                tamanio_a_reservar = tamanioDireccionesFisicas(operacionIn->direccionesFisicas, operacionIn->cantidad_direccionesFisicas); 
                valor = malloc(tamanio_a_reservar);
                offset = 0;
                while(tamanio_a_reservar > 0) {
                    tamanio_a_reservar -= 1;
                    char auxiliar = '\0';
                    memcpy(valor + offset, &auxiliar, sizeof(char));
                    offset += sizeof(char);
                }
                int tamanio_stdin = string_length(string);
                if(tamanio_stdin > tamanio_a_reservar) {
                    tamanio_stdin = tamanio_a_reservar;
                }
                memcpy(valor, string, tamanio_stdin);
                hacerOperacionEscrituraMemoria(operacionIn->direccionesFisicas, valor, operacionIn->pid, conexion_memoria);
                // Enviar confirmación al Kernel
                buffer_confirmacion = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer_confirmacion, P_IO_STDIN_READ, conexion_kernel);
                queue_destroy(operacionIn->direccionesFisicas);
                free(operacionIn);
                free(valor);
                break;
            case P_IO_FS_CREATE:
                usleep(configuracion.TIEMPO_UNIDAD_TRABAJO * 1000);
                operacionFSCREADEL * operacionFSC = deserializarOperacionFSCREADEL(paquete_kernel->buffer);
                mensaje = string_from_format ("PID: %d - Operacion: %s", operacionFSC->pid, "IO_FS_CREATE");
                logInfoSincronizado(mensaje);
                free(mensaje);
                mensaje = string_from_format ("PID: %d - Crear Archivo: %s", operacionFSC->pid, operacionFSC->nombre_archivo);
                logInfoSincronizado(mensaje);
                free(mensaje);
                if(hayEspacioLibre(1)){
                    char * ruta_archivo = string_duplicate(configuracion.PATH_BASE_DIALFS);
                    string_append(&ruta_archivo, "/");
                    string_append(&ruta_archivo, operacionFSC->nombre_archivo);
                    FILE * archivo = fopen(ruta_archivo, "a+");
                    fclose(archivo);
                    int numero_bloque = obtenerBloqueLibre(); 
                    t_config * modificar_archivo = config_create(ruta_archivo);
                    char * string_bloqueInicial = string_itoa(numero_bloque);
                    config_set_value(modificar_archivo, "BLOQUE_INICIAL", string_bloqueInicial);
                    free(string_bloqueInicial);
                    fseek(bitmap, 0, SEEK_SET);
                    fwrite(bitmap_archivos->bitarray, configuracion.BLOCK_COUNT/8, 1, bitmap);
                    fclose(bitmap);
                    bitmap = fopen(ruta_bitmap, "r+");
                    config_set_value(modificar_archivo, "TAMANIO_ARCHIVO", "0");
                    config_save(modificar_archivo);
                    config_destroy(modificar_archivo);
                    free(ruta_archivo);
                    list_add(archivos_dial_fs, string_duplicate(operacionFSC->nombre_archivo));
                }
                buffer_confirmacion = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer_confirmacion, P_IO_FS_CREATE, conexion_kernel);
                free(operacionFSC->nombre_archivo);
                free(operacionFSC);
                break;
            case P_IO_FS_DELETE:
                usleep(configuracion.TIEMPO_UNIDAD_TRABAJO * 1000);
                operacionFSCREADEL * operacionFSD = deserializarOperacionFSCREADEL(paquete_kernel->buffer);
                mensaje = string_from_format ("PID: %d - Operacion: %s", operacionFSD->pid, "IO_FS_DELETE");
                logInfoSincronizado(mensaje);
                free(mensaje);
                mensaje = string_from_format ("PID: %d - Eliminar Archivo: %s", operacionFSD->pid, operacionFSD->nombre_archivo);
                logInfoSincronizado(mensaje);
                free(mensaje);
                char * ruta_archivo = string_duplicate(configuracion.PATH_BASE_DIALFS);
                string_append(&ruta_archivo, "/");
                string_append(&ruta_archivo, operacionFSD->nombre_archivo);
                t_config * archivo = config_create(ruta_archivo);
                int tamanioActual = config_get_int_value(archivo, "TAMANIO_ARCHIVO");
                int cantidad_bloques_actual = calcularCantidadBloques(tamanioActual);
                int bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
                int numeroBloqueActual = bloque_inicial + cantidad_bloques_actual -1;
                for(int i = numeroBloqueActual; i > bloque_inicial-1; i--) {
                    bitarray_set_bit(bitmap_archivos, i);
                }
                fseek(bitmap, 0, SEEK_SET);
                fwrite(bitmap_archivos->bitarray, configuracion.BLOCK_COUNT/8, 1, bitmap);
                fclose(bitmap);
                bitmap = fopen(ruta_bitmap, "r+");
                buffer_confirmacion = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer_confirmacion, P_IO_FS_DELETE, conexion_kernel);
                config_destroy(archivo);
                
                nombre_a_borrar = string_duplicate(operacionFSD->nombre_archivo);
                char * nombre_archivo = list_find(archivos_dial_fs, compararNombreArchivo);
                list_remove_element(archivos_dial_fs, nombre_archivo);
                free(nombre_archivo);
                free(nombre_a_borrar);
                remove(ruta_archivo);
                free(ruta_archivo);
                free(operacionFSD->nombre_archivo);
                free(operacionFSD);
                break;
            case P_IO_FS_TRUNCATE:
                usleep(configuracion.TIEMPO_UNIDAD_TRABAJO * 1000);
                operacionFSTRUN * operacionSTRUN = deserializarOperacionFSTRUN(paquete_kernel->buffer);
                mensaje = string_from_format ("PID: %d - Operacion: %s", operacionSTRUN->pid, "IO_FS_TRUNCATE");
                logInfoSincronizado(mensaje);
                free(mensaje);

                mensaje = string_from_format ("PID: %d - Truncar Archivo: %s - Tamaño: %d", operacionSTRUN->pid, operacionSTRUN->nombre_archivo, operacionSTRUN->tamanio_archivo);
                logInfoSincronizado(mensaje);
                free(mensaje);
                ruta_archivo = string_duplicate(configuracion.PATH_BASE_DIALFS);
                string_append(&ruta_archivo, "/");
                string_append(&ruta_archivo, operacionSTRUN->nombre_archivo);
                archivo = config_create(ruta_archivo);
                tamanioActual = config_get_int_value(archivo, "TAMANIO_ARCHIVO");
                int deltaTamanio =  operacionSTRUN->tamanio_archivo - tamanioActual;
                int cantidad_bloques_nuevos = calcularCantidadBloques(abs(deltaTamanio));
                cantidad_bloques_actual = calcularCantidadBloques(tamanioActual);
                bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
                numeroBloqueActual = bloque_inicial + cantidad_bloques_actual -1;
                config_destroy(archivo);
                if(deltaTamanio > 0) {
                    if(hayEspacioLibre(cantidad_bloques_nuevos)){
                        if(!hayEspacioLibreContiguo(bloque_inicial, cantidad_bloques_nuevos)) {
                            mensaje = string_from_format ("PID: %d - Inicio Compactación.", operacionSTRUN->pid);
                            logInfoSincronizado(mensaje);
                            free(mensaje);

                            compactar(operacionSTRUN->nombre_archivo);
                            
                            mensaje = string_from_format ("PID: %d - Fin Compactación.", operacionSTRUN->pid);
                            logInfoSincronizado(mensaje);
                            free(mensaje);
                        }
                        archivo = config_create(ruta_archivo);
                        for(int i = numeroBloqueActual + 1 ; i < numeroBloqueActual + cantidad_bloques_nuevos; i++) {
                            bitarray_clean_bit(bitmap_archivos, i);
                        }
                        char * tamanio_nuevo = string_itoa(operacionSTRUN->tamanio_archivo);
                        config_set_value(archivo, "TAMANIO_ARCHIVO", tamanio_nuevo);
                        free(tamanio_nuevo);
                        config_save(archivo);
                        config_destroy(archivo);
                    }
                } else if(deltaTamanio < 0) {
                    archivo = config_create(ruta_archivo);
                    for(int i = numeroBloqueActual; i > numeroBloqueActual - cantidad_bloques_nuevos + 1; i--) {
                        bitarray_set_bit(bitmap_archivos, i);
                    }
                    char * tamanio_nuevo = string_itoa(operacionSTRUN->tamanio_archivo);
                    config_set_value(archivo, "TAMANIO_ARCHIVO", tamanio_nuevo);
                    free(tamanio_nuevo);
                    config_save(archivo);
                    config_destroy(archivo);
                }
                fseek(bitmap, 0, SEEK_SET);
                fwrite(bitmap_archivos->bitarray, configuracion.BLOCK_COUNT/8, 1, bitmap);
                fclose(bitmap);
                bitmap = fopen(ruta_bitmap, "r+");
                fclose(bloques);
                bloques = fopen(ruta_bloques, "r+");
                buffer_confirmacion = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer_confirmacion, P_IO_FS_TRUNCATE, conexion_kernel);
                free(ruta_archivo);
                free(operacionSTRUN->nombre_archivo);
                free(operacionSTRUN);
                break;
            case P_IO_FS_WRITE:
                usleep(configuracion.TIEMPO_UNIDAD_TRABAJO * 1000);
                operacionFSWR * operacionW = deserializarOperacionFSWR(paquete_kernel->buffer);
                mensaje = string_from_format ("PID: %d - Operacion: %s", operacionW->pid, "IO_FS_WRITE");
                logInfoSincronizado(mensaje);
                free(mensaje);
                tamanio_a_reservar = tamanioDireccionesFisicas(operacionW->direccionesFisicas, operacionW->cantidad_direccionesFisicas); 
                mensaje = string_from_format ("PID: %d - Escribir Archivo: %s - Tamaño a Escribir: %d - Puntero Archivo: %d", operacionW->pid, operacionW->nombre_archivo, tamanio_a_reservar, operacionW->punteroArchivo);
                logInfoSincronizado(mensaje);
                free(mensaje);
                
                valor = malloc(tamanio_a_reservar);
                void * valor_leido_fs = hacerOperacionLecturaMemoria(operacionW->direccionesFisicas, valor, operacionW->pid, conexion_memoria);


                ruta_archivo = string_duplicate(configuracion.PATH_BASE_DIALFS);
                string_append(&ruta_archivo, "/");
                string_append(&ruta_archivo, operacionW->nombre_archivo);
                archivo = config_create(ruta_archivo);
                bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
                fseek(bloques, bloque_inicial*configuracion.BLOCK_SIZE + operacionW->punteroArchivo, SEEK_SET);
                fwrite(valor_leido_fs, tamanio_a_reservar, 1, bloques);
                
                fclose(bloques);
                bloques = fopen(ruta_bloques, "r+");
                buffer_confirmacion = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer_confirmacion, P_IO_FS_WRITE, conexion_kernel);
                config_destroy(archivo);
                free(valor_leido_fs);
                free(ruta_archivo);
                free(operacionW->nombre_archivo);
                queue_destroy(operacionW->direccionesFisicas);
                free(operacionW);
                break;
            case P_IO_FS_READ:
                usleep(configuracion.TIEMPO_UNIDAD_TRABAJO * 1000);
                operacionFSWR * operacionR = deserializarOperacionFSWR(paquete_kernel->buffer);
                mensaje = string_from_format ("PID: %d - Operacion: %s", operacionR->pid, "IO_FS_READ");
                logInfoSincronizado(mensaje);
                free(mensaje);
                tamanio_a_reservar = tamanioDireccionesFisicas(operacionR->direccionesFisicas, operacionR->cantidad_direccionesFisicas); 
                mensaje = string_from_format ("PID: %d - Leer Archivo: %s - Tamaño a Leer: %d - Puntero Archivo: %d", operacionR->pid, operacionR->nombre_archivo, tamanio_a_reservar, operacionR->punteroArchivo);
                logInfoSincronizado(mensaje);
                free(mensaje);             

                ruta_archivo = string_duplicate(configuracion.PATH_BASE_DIALFS);
                string_append(&ruta_archivo, "/");
                string_append(&ruta_archivo, operacionR->nombre_archivo);
                archivo = config_create(ruta_archivo);
                bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
                valor = malloc(tamanio_a_reservar);
                fseek(bloques, bloque_inicial*configuracion.BLOCK_SIZE + operacionR->punteroArchivo, SEEK_SET);
                fread(valor, tamanio_a_reservar, 1, bloques);

                hacerOperacionEscrituraMemoria(operacionR->direccionesFisicas, valor, operacionR->pid, conexion_memoria);

                buffer_confirmacion = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer_confirmacion, P_IO_FS_READ, conexion_kernel);
                config_destroy(archivo);
                free(ruta_archivo);
                free(valor);
                free(operacionR->nombre_archivo);
                queue_destroy(operacionR->direccionesFisicas);
                free(operacionR);
                break;
            default: 
                fprintf(stderr, "No se encontro la operacion");            
        }
        eliminar_paquete(paquete_kernel);
    }
    
    if(tipo_interfaz->tipo == DIALFS) {
        fclose(bitmap);
        fclose(bloques);
        free(ruta_bitmap);
        free(ruta_bloques);
    }
    free(tipo_interfaz->nombre);
    free(tipo_interfaz);
    config_destroy(config);
    free(direccion_config);
    return 0;
}

