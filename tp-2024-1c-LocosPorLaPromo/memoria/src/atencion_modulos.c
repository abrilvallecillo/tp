#include "atencion_modulos.h"
#include <commons/collections/list.h>
#include <string.h>
#include <commons/string.h>
#include "configuracion.h"
#include <utils/logger_concurrente.h>

void * atenderPeticiones(void * conexion_cliente) {
    int * socket_cliente = (int *) conexion_cliente;
    int resultado = recibirHandshake(*socket_cliente);
    
    if(resultado == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con el cliente");
        abort();
    }

    while(1) {
        t_paquete * paquete_peticion = recibirPaqueteGeneral(*socket_cliente);
        
        if(paquete_peticion == NULL) {break;}
        
        usleep(configuracion.RETARDO_RESPUESTA * 1000);
        t_buffer * buffer;
        
        switch(paquete_peticion->codigoOperacion) {
            
            case CREAR_PROCESO:
                inicializar_proceso * nueva_peticion = deserializarInicializarProceso(paquete_peticion->buffer);
                pcb * proceso_creado = CrearProceso(nueva_peticion);
                
                if(proceso_creado) {
                    buffer = serializarProceso(proceso_creado);
                    enviarBufferProcesoConMotivo(buffer, PROCESO_CREADO, *socket_cliente);
                    free(proceso_creado);
                
                } else {
                    buffer = crearBufferGeneral(0);
                    enviarBufferProcesoConMotivo(buffer, CREAR_PROCESO, *socket_cliente);
                }
                
                free(nueva_peticion->direccion_codigo);
                free(nueva_peticion);
                break;
            
            case PETICION_INSTRUCCION:
                peticion_instruccion * nueva_instruccion = deserializarPeticionInstruccion(paquete_peticion->buffer);
                char * string_instruccion = buscarInstruccion(nueva_instruccion);
               
                enviarSegunErrorAlBuscarInstr(buffer, string_instruccion, *socket_cliente);
               
                free(nueva_instruccion);
                break;
            
            case P_RESIZE:
                peticion_resize * nuevo_resize = deserializarPeticionResize(paquete_peticion->buffer);
                codigos_operacion resultado = redimensionarProceso(nuevo_resize);
               
                buffer = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer, resultado, *socket_cliente);
               
                free(nuevo_resize);
                break;
           
            case ESCRIBIR_MEMORIA:
                t_operacionMemEscribirUsuario * operacion_escritura = deserializarOperacionMemEscribirUsuario(paquete_peticion->buffer);
                char * mensaje = string_from_format ("PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Tamaño %d", operacion_escritura->pid, operacion_escritura->direccion->direccionFisica, operacion_escritura->direccion->tamanioEnvio);
                logInfoSincronizado(mensaje);
                free(mensaje);
               
                pthread_mutex_lock(mutexMemoria);
                memcpy(memoria+operacion_escritura->direccion->direccionFisica, operacion_escritura->valor, operacion_escritura->direccion->tamanioEnvio);
                pthread_mutex_unlock(mutexMemoria);
               
                buffer = crearBufferGeneral(0);
                enviarBufferProcesoConMotivo(buffer, ESCRIBIR_MEMORIA, *socket_cliente);
               
                free(operacion_escritura->direccion);
                free(operacion_escritura->valor);
                free(operacion_escritura);
                break;
           
            case LEER_MEMORIA:
                t_operacionMemLeerUsuario * operacion_lectura = deserializarOperacionMemLeerUsuario(paquete_peticion->buffer);
                mensaje = string_from_format ("PID: %d - Accion: LEER - Direccion fisica: %d - Tamaño %d", operacion_lectura->pid, operacion_lectura->direccion->direccionFisica, operacion_lectura->direccion->tamanioEnvio);
                logInfoSincronizado(mensaje);
                free(mensaje);

                void * valor = malloc(operacion_lectura->direccion->tamanioEnvio);
               
                pthread_mutex_lock(mutexMemoria);
                memcpy(valor, memoria+operacion_lectura->direccion->direccionFisica, operacion_lectura->direccion->tamanioEnvio);
                pthread_mutex_unlock(mutexMemoria);

                buffer = crearBufferGeneral(operacion_lectura->direccion->tamanioEnvio);
                agregarABufferVoidP(buffer, valor, operacion_lectura->direccion->tamanioEnvio);
                enviarBufferProcesoConMotivo(buffer, LEER_MEMORIA, *socket_cliente);
               
                free(operacion_lectura->direccion);
                free(operacion_lectura);
                free(valor);
                break;
            
            case PEDIDO_MARCO:
                t_pedidoMarco * pedido = deserializarPedidoMarco(paquete_peticion->buffer);
                t_tablaPaginas * tabla = buscarTablaDePaginas(pedido->pid);
                nro_pagina_a_comparar_tabla = pedido->numeroPagina;
                fila_tabla_de_paginas * fila = list_find(tabla->listaPaginas, compararValidezYNroPagina);
                
                if(fila == NULL){
                    buffer = crearBufferGeneral(0);
                    enviarBufferProcesoConMotivo(buffer, OUT_OF_MEMORY, *socket_cliente);
               
                } else {
                    char * mensaje = string_from_format ("PID: %d - Pagina: %d - Marco: %d", pedido->pid, pedido->numeroPagina, fila->numeroMarco);
                    logInfoSincronizado(mensaje);
                    free(mensaje);
                    
                    buffer = serializarMarco(fila->numeroMarco);
                    enviarBufferProcesoConMotivo(buffer, PEDIDO_MARCO, *socket_cliente);
                }
               
                free(pedido);
                break;
          
            case P_BORRAR_MEMORIA:
                uint32_t pid = deserializarBorrarMemoriaP(paquete_peticion->buffer);
                tabla = buscarTablaDePaginas(pid);
                borrarTablaDePaginas(tabla);
                borrarCodigoProceso(pid);
                break;
           
            case PEDIR_TAM_PAG:
                logInfoSincronizado("Me llego!");
                buffer = serializarTamPagina(configuracion.TAMANIO_PAGINAS);
                enviarBufferProcesoConMotivo(buffer, PEDIR_TAM_PAG, *socket_cliente);
                break;
           
           default:
                fprintf(stderr, "Error: codigo de operacion desconocido");
                break;
        }
        eliminar_paquete(paquete_peticion);
    }
   
    logInfoSincronizado("Desconectado");
    liberarConexionCliente(*socket_cliente);
    free(socket_cliente);
    return NULL;
}

// Creación de proceso
// Esta petición podrá venir solamente desde el módulo Kernel, y el módulo Memoria deberá crear las estructuras administrativas necesarias.

pcb * CrearProceso(inicializar_proceso * peticion) {
    if(peticion->direccion_codigo == NULL) {return NULL;}
    pcb * pcb_nuevo = malloc(sizeof(pcb));
    codigo_proceso * codigo_nuevo = malloc(sizeof(codigo_proceso));

    pcb_nuevo->PID = peticion->pid;
    pcb_nuevo->PC = 0;
    pcb_nuevo->registros.AX = 0;
    pcb_nuevo->registros.BX = 0;
    pcb_nuevo->registros.CX = 0;
    pcb_nuevo->registros.DX = 0;
    pcb_nuevo->registros.EAX = 0;
    pcb_nuevo->registros.EBX = 0;
    pcb_nuevo->registros.ECX = 0;
    pcb_nuevo->registros.EDX = 0; 
    pcb_nuevo->Quantum = peticion->quantum;
    codigo_nuevo->pid = peticion->pid;

    char * direccion = string_duplicate(configuracion.PATH_INSTRUCCIONES);
    string_append(&direccion, peticion->direccion_codigo);
    FILE *codigo_proceso_archivo = fopen(direccion, "r");
    char *lectura_linea = malloc(256 * sizeof(char));
   
    if (codigo_proceso_archivo == NULL){
        fprintf(stderr, "El archivo esta vacio\n");
        free(pcb_nuevo);
        free(codigo_nuevo);
        free(direccion);
        return NULL;
    }
  
    codigo_nuevo->array_instrucciones = string_array_new();
    
    while(fgets(lectura_linea, 256, codigo_proceso_archivo)){
        string_array_push(&codigo_nuevo->array_instrucciones, string_duplicate(lectura_linea));
    }
    
    fclose(codigo_proceso_archivo);
    free(direccion);
    agregarElementoAListaSincronizada(codigos_programas, codigo_nuevo);
    t_tablaPaginas * tablaPaginas = malloc(sizeof(t_tablaPaginas));
    tablaPaginas->pid = peticion->pid;
    tablaPaginas->listaPaginas = list_create();
    agregarElementoAListaSincronizada(tablaPaginasProcesos, tablaPaginas);
    
    // Creación / destrucción de Tabla de Páginas: “PID: <PID> - Tamaño: <CANTIDAD_PAGINAS>”
    char * mensaje = string_from_format("PID: %d - Tamaño: %d", peticion->pid, 0);
    logInfoSincronizado(mensaje);
    free(mensaje);
    free(lectura_linea);
    return pcb_nuevo;   
}

char * buscarInstruccion(peticion_instruccion * nueva_instruccion){
    pthread_mutex_lock(codigos_programas->mutex_lista);
    pid_solicitado = nueva_instruccion->pid;
    codigo_proceso * codigo_del_programa = list_find(codigos_programas->lista, pid_a_comparar);
    pthread_mutex_unlock(codigos_programas->mutex_lista);
    
    if(codigo_del_programa){
        int pc_instruccion = nueva_instruccion->pc;
        return codigo_del_programa->array_instrucciones[pc_instruccion];
    } else {
        return NULL;
    }
}

bool pid_a_comparar(void* codigo_del_programa){
    codigo_proceso * codigo = (codigo_proceso*) codigo_del_programa;
    return pid_solicitado == codigo->pid;
}

t_instruccion * estandarizarStringInstruccion(char * string_instruccion){
    t_instruccion * instruccion_estandarizada = malloc(sizeof(t_instruccion));
    instruccion_estandarizada->longitud = string_length(string_instruccion)+1;
    instruccion_estandarizada->string_instruccion = string_duplicate(string_instruccion);
    return instruccion_estandarizada;
}

void enviarSegunErrorAlBuscarInstr(t_buffer * buffer, char * string_instruccion, int socket_cliente) {
    if(string_instruccion) {
       t_instruccion * instruccion_a_mandar = estandarizarStringInstruccion(string_instruccion);                
        buffer = serializarInstruccion(instruccion_a_mandar);
        enviarBufferPorPaquete(buffer, ENVIAR_INSTRUCCION, socket_cliente);
        free(instruccion_a_mandar->string_instruccion);
        free(instruccion_a_mandar);
    } else {
        buffer = crearBufferGeneral(0);
        enviarBufferPorPaquete(buffer, P_ERROR, socket_cliente);
    }
}

// Ajustar tamaño de un proceso
// Al llegar una solicitud de ajuste de tamaño de proceso (resize) se deberá cambiar el tamaño del proceso de acuerdo al nuevo tamaño. Se pueden dar 2 opciones:

// Ampliación de un proceso
// Se deberá ampliar el tamaño del proceso al final del mismo, pudiendo solicitarse múltiples páginas. Es posible que en un punto no se puedan solicitar más marcos ya que la memoria se encuentra llena, por lo que en ese caso se deberá contestar con un error de Out Of Memory.

// Reducción de un proceso
// Se reducirá el mismo desde el final, liberando, en caso de ser necesario, las páginas que ya no sean utilizadas (desde la última hacia la primera).

codigos_operacion redimensionarProceso(peticion_resize * peticion){
    int cantidadPaginasNuevas = peticion->tamanio / configuracion.TAMANIO_PAGINAS;
    
    if(peticion->tamanio % configuracion.TAMANIO_PAGINAS && peticion->tamanio){ cantidadPaginasNuevas++; }
    
    pthread_mutex_lock(mutexMarcosLibres);
        int deltaCantidadPaginas = cantidadPaginasNuevas - cantidadTablaPaginas(peticion->pid);
        
        if(deltaCantidadPaginas > 0) {
            int cantidad_marcos = contarMarcosLibres();
            char * mensaje_temp = string_from_format("Cantidad Marcos: %d", cantidad_marcos);
            logInfoSincronizado(mensaje_temp);
            free(mensaje_temp);
        
            if (deltaCantidadPaginas > contarMarcosLibres()) { return OUT_OF_MEMORY; }
    pthread_mutex_unlock(mutexMarcosLibres);
        
            t_tablaPaginas * tabla = buscarTablaDePaginas(peticion->pid);
            char * mensaje = string_from_format ("PID: %d - Tamaño Actual: %d - Tamaño a Ampliar: %d", tabla->pid, cantidadTablaPaginas(peticion->pid)*configuracion.TAMANIO_PAGINAS, peticion->tamanio);
            logInfoSincronizado(mensaje);
            free(mensaje);
        
            while (deltaCantidadPaginas > 0){
                fila_tabla_de_paginas * fila = malloc(sizeof(fila_tabla_de_paginas));
                fila_tabla_de_paginas * ultimaFila = buscarUltimaFilaValida(tabla);
        
                if(ultimaFila == NULL){ fila->numeroPagina = 0; } 
                else { fila->numeroPagina = ultimaFila->numeroPagina +1;}

                fila->numeroMarco = encontrarPrimerMarcoLibre();
                fila->validez = true;
                deltaCantidadPaginas--;
                bitarray_clean_bit(marcosLibres, fila->numeroMarco);
            
                if(ultimaFila == NULL || ultimaFila->numeroPagina == cantidadTablaPaginas(peticion->pid) - 1) { 
                    list_add(tabla->listaPaginas, fila);
                } else {
                    fila_tabla_de_paginas *  fila_vieja = list_replace(tabla->listaPaginas, fila->numeroPagina, fila);
                    free(fila_vieja);
                }
            }

        } else if(deltaCantidadPaginas < 0){
            t_tablaPaginas * tabla = buscarTablaDePaginas(peticion->pid);
            char * mensaje = string_from_format ("PID: %d - Tamaño Actual: %d - Tamaño a Reducir: %d", tabla->pid, cantidadTablaPaginas(peticion->pid)*configuracion.TAMANIO_PAGINAS, peticion->tamanio);
            logInfoSincronizado(mensaje);
            free(mensaje);
        
            while (deltaCantidadPaginas < 0){
                fila_tabla_de_paginas * ultimaFila = buscarUltimaFilaValida(tabla);
                ultimaFila->validez = false;
                deltaCantidadPaginas++;
                bitarray_set_bit(marcosLibres, ultimaFila->numeroMarco);
            }
        }
  
    pthread_mutex_unlock(mutexMarcosLibres);
    return P_RESIZE;
}

bool compararValidezYNroPagina(void * elemento){
    fila_tabla_de_paginas * fila = (fila_tabla_de_paginas*) elemento;
    return nro_pagina_a_comparar_tabla == fila->numeroPagina && fila->validez;
}

// Finalización de proceso
// Esta petición podrá venir solamente desde el módulo Kernel. 
// El módulo Memoria, al ser finalizado un proceso, debe liberar su espacio de memoria (marcando los frames como libres pero sin sobreescribir su contenido).

void borrarTablaDePaginas(t_tablaPaginas *  tabla) {

    pthread_mutex_lock(tablaPaginasProcesos->mutex_lista);
        list_remove_element(tablaPaginasProcesos->lista, tabla);
    pthread_mutex_unlock(tablaPaginasProcesos->mutex_lista);
    
    int tamanio = 0;
        
    if(list_size(tabla->listaPaginas) > 0) {
        t_list * lista_paginas_validas = list_filter(tabla->listaPaginas, filaValida);
        tamanio = list_size(lista_paginas_validas);
        list_destroy(lista_paginas_validas);
    }
    
    char * mensaje = string_from_format("PID: %d - Tamaño: %d", tabla->pid, tamanio * configuracion.TAMANIO_PAGINAS);
    logInfoSincronizado(mensaje);
    free(mensaje);

    pthread_mutex_lock(mutexMarcosLibres); 
        if(!list_is_empty(tabla->listaPaginas)) { list_clean_and_destroy_elements(tabla->listaPaginas, eliminar_fila); }
    pthread_mutex_unlock(mutexMarcosLibres);

    list_destroy(tabla->listaPaginas);
    free(tabla);
}

void eliminar_fila(void * elemento) {
    fila_tabla_de_paginas * fila = (fila_tabla_de_paginas *) elemento;
    if(fila->validez){ bitarray_set_bit(marcosLibres, fila->numeroMarco); }        
    free(fila);
}

void borrarCodigoProceso(uint32_t pid) {

    pthread_mutex_lock(codigos_programas->mutex_lista);
        pid_solicitado = pid;
        codigo_proceso * codigo_del_programa = list_find(codigos_programas->lista, pid_a_comparar);
        list_remove_element(codigos_programas->lista, codigo_del_programa);
    pthread_mutex_unlock(codigos_programas->mutex_lista);

    string_array_destroy(codigo_del_programa->array_instrucciones);
    free(codigo_del_programa);
}

