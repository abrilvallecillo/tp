#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <utils/hilos.h>
#include <utils/conexiones.h>
#include <commons/log.h>
#include <commons/config.h> 
#include <pthread.h>
#include <utils/procesos.h>
#include "atencion_kernel.h"
#include "cpu.h"
#include "ciclo_instruccion.h"
#include "configuracion.h"
#include "tlb.h"
#include <utils/logger_concurrente.h>
#include <commons/string.h>

void accionarSegunInterrupcion(pcb* proceso_elegido, int conexion_dispatch_kernel);
void pedirYSetearTamanioPagina();
void inicializarRegistrosPropios();

int main(int argc, char* argv[]) {
    //Declaracion de loggers y config
    t_log* logger_debug;
    t_config* config;

    //Creacion de los loggers
    logger_debug = log_create("cpu_debug.log", "Log para puebas", true, LOG_LEVEL_INFO);
    char * direccion_config;

    if(argc < 2) { direccion_config = string_duplicate("cpu.config");
    } else { direccion_config = string_duplicate(argv[1]);
    } obtener_config(direccion_config);
    
    estado_interrupcion = crearEnteroSincronizado();
    pid_a_finalizar = crearEnteroSincronizado();
    modificarEnteroSincronizado(pid_a_finalizar, -1);
    modificarEnteroSincronizado(estado_interrupcion, -1);
    interrupcion_proceso = NO_HAY_INTERRUPCION;
    cola_parametros = queue_create();
    consumirFinQ = crearEnteroSincronizado();
    modificarEnteroSincronizado(consumirFinQ, 0);

    //Inicializamos la TLB
    TLB = list_create();
    inicializarLogger("cpu.log", "Log del CPU obligatorio");

    //Creacion del socket de la conexion a memoria
    log_info(logger_debug, "Conectando a memoria...");
    conexion_memoria = crearConexionCliente(configuracion.IP_MEMORIA, configuracion.PUERTO_MEMORIA);
    int handshake_memoria = enviarHandshake(conexion_memoria, CPU);
    if(handshake_memoria == 0) { log_info(logger_debug, "Handshake exitoso");
    } else { log_error(logger_debug, "No se pudo realizar el handshake"); };
     pedirYSetearTamanioPagina();
    //Creacion de socket de la conexion dispatch con kernel
    
    int conexion_dispatch = crearConexionServidor(configuracion.PUERTO_ESCUCHA_DISPATCH);
    
    //Conexion dispatch
    conexion_dispatch_kernel = esperarCliente(conexion_dispatch);
    int resultado_dispatch = recibirHandshake(conexion_dispatch_kernel);
    if(resultado_dispatch == 0) { log_info(logger_debug, "Handshake exitoso");
    } else { log_error(logger_debug, "No se pudo realizar el handshake");};

    //Creo hilo detachable para gestionar las interrupciones
    pthread_t escucha_interrupt = crearHilo(atenderInterrupciones, NULL);
    pthread_detach(escucha_interrupt);
    log_info(logger_debug, "Servidor listo para recibir al cliente");
    
    inicializarRegistrosPropios();

    while(1){
        t_paquete* buffer_proceso = recibirPaqueteGeneral(conexion_dispatch_kernel);
        modificarEnteroSincronizado(consumirFinQ, 0);
       
        if(buffer_proceso == NULL) {
            fprintf(stderr, "Hubo problemas con el kernel");
            abort();
        }
        
        pcb* proceso_elegido = deserializarProceso(buffer_proceso->buffer);
        cargarRegistrosEnCPU(proceso_elegido);
        cicloDeInstruccion(proceso_elegido->PID, buffer_proceso->codigoOperacion);
        cargarRegistrosEnPCB(proceso_elegido);
        accionarSegunInterrupcion(proceso_elegido, conexion_dispatch_kernel);
        free(proceso_elegido);
        eliminar_paquete(buffer_proceso);        
    }

    log_destroy(logger_debug);
    config_destroy(config);
    return 0;
}

void accionarSegunInterrupcion(pcb* proceso_elegido, int conexion_dispatch_kernel) {
    t_buffer * buffer;
    switch(interrupcion_proceso) {
            
            case INT_FIN_QUANTUM:
                buffer = serializarProceso(proceso_elegido);
                break;
            
            case P_IO_GEN_SLEEP:
                pcb_IOGENSLEEP * nuevo_IO_GEN_SLEEP = crearPCB_IOGENSLEEP(proceso_elegido);
                buffer = serializarProcesoIOGENSLEEP(nuevo_IO_GEN_SLEEP);
                free(nuevo_IO_GEN_SLEEP->nombre_interfaz);
                free(nuevo_IO_GEN_SLEEP);
                break;
            
            case INT_FIN_CONSOLA:
                buffer = serializarProceso(proceso_elegido);
                break;
            
            case OUT_OF_MEMORY:
                buffer = serializarProceso(proceso_elegido);
                break;
            
            case P_SIGNAL:
            
            case P_WAIT:
                pcb_recurso * nuevo_precurso = crearPCBRecurso(proceso_elegido);
                buffer = serializarProcesoRecurso(nuevo_precurso);
                free(nuevo_precurso->nombre_recurso);
                free(nuevo_precurso);
                break;
            
            case P_IO_STDIN_READ:
            
            case P_IO_STDOUT_WRITE:
                pcb_IOSTD_IN_OUT * nuevo_STDINOUT = crearPCB_STDINOUT(proceso_elegido);
                buffer = serializarProcesoIOSTDINOUT(nuevo_STDINOUT);
                free(nuevo_STDINOUT->nombre_interfaz);
                queue_destroy(nuevo_STDINOUT->direccionesFisicas);
                free(nuevo_STDINOUT);
                break;
            
            case P_EXIT:
                buffer = serializarProceso(proceso_elegido);
                break;
            
            case P_IO_FS_CREATE:
            
            case P_IO_FS_DELETE:
                pcb_FSCREADEL * nuevo_FSCREADEL = crearPCB_FSCREADEL(proceso_elegido);
                buffer = serializarProcesoFSCREADEL(nuevo_FSCREADEL);
                free(nuevo_FSCREADEL->nombre_archivo);
                free(nuevo_FSCREADEL->nombre_interfaz);
                free(nuevo_FSCREADEL);
                break;
            
            case P_IO_FS_TRUNCATE:
                pcb_FSTRUN * nuevo_FSTRUN = crearPCB_FSTRUN(proceso_elegido);
                buffer = serializarProcesoFSTRUN(nuevo_FSTRUN);
                free(nuevo_FSTRUN->nombre_archivo);
                free(nuevo_FSTRUN->nombre_interfaz);
                free(nuevo_FSTRUN);
                break;
            
            case P_IO_FS_WRITE:
            
            case P_IO_FS_READ:
                pcb_FSWR * nuevo_FSWR = crearPCB_FSWR(proceso_elegido);
                buffer = serializarProcesoFSWR(nuevo_FSWR);
                queue_destroy(nuevo_FSWR->direccionesFisicas);
                free(nuevo_FSWR->nombre_archivo);
                free(nuevo_FSWR->nombre_interfaz);
                free(nuevo_FSWR);
                break;
            
            default:
                fprintf(stderr, "Error. INT no reconocida.");           
    }

    enviarBufferProcesoConMotivo(buffer, interrupcion_proceso, conexion_dispatch_kernel);
}

void pedirYSetearTamanioPagina() {
    t_buffer * buffer = crearBufferGeneral(0);
    enviarBufferPorPaquete(buffer, PEDIR_TAM_PAG, conexion_memoria);
    t_paquete * paq_tam_pag = recibirPaqueteGeneral(conexion_memoria);
    tamanioPagina = deserializarTamPagina(paq_tam_pag->buffer);
    eliminar_paquete(paq_tam_pag);
}

void inicializarRegistrosPropios() {
    registrosPropios.generales.AX = 0;
    registrosPropios.generales.BX = 0;
    registrosPropios.generales.CX = 0;
    registrosPropios.generales.DX = 0;
    registrosPropios.generales.EAX = 0;
    registrosPropios.generales.EBX = 0;
    registrosPropios.generales.ECX = 0;
    registrosPropios.generales.EDX = 0;
    registrosPropios.DI = 0;
    registrosPropios.SI = 0;
    registrosPropios.PC = 0;
}