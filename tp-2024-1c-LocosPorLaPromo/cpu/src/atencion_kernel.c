#include "atencion_kernel.h"
#include <utils/hello.h>
#include <utils/conexiones.h>
#include "configuracion.h"
#include "cpu.h"

void * atenderInterrupciones(void * sin_parametro){
    int conexion_interrupt = crearConexionServidor(configuracion.PUERTO_ESCUCHA_INTERRUPT);
    int conexion_interrupt_kernel = esperarCliente(conexion_interrupt);
    int resultado_interrupt = recibirHandshake(conexion_interrupt_kernel);
    
    if(resultado_interrupt == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con la kernel en interrupt");
        abort();
    }
    
    while(1){
        t_paquete * paquete_interrupcion = recibirPaqueteGeneral(conexion_interrupt_kernel);
        
        if(paquete_interrupcion == NULL) {
            fprintf(stderr, "Problemas al recibir interrupciones del kernel");
            abort();
        }
       
        int interrupcion = leerEnteroSincronizado(estado_interrupcion);
        
        if(!esInterrupcionSincronica(interrupcion)){
            uint32_t pid = leerDeBufferUint32(paquete_interrupcion->buffer);
            
            if(interrupcion != INT_FIN_CONSOLA || pid != leerEnteroSincronizado(pid_a_finalizar)){
               
                if(paquete_interrupcion->codigoOperacion != INT_FIN_QUANTUM || leerEnteroSincronizado(consumirFinQ) == 0){
                    modificarEnteroSincronizado(pid_a_finalizar, pid);
                    modificarEnteroSincronizado(estado_interrupcion, paquete_interrupcion->codigoOperacion);
                } else {
                    modificarEnteroSincronizado(consumirFinQ, 0);
                } 

            }
        } 

        eliminar_paquete(paquete_interrupcion);
    }
    
    liberarConexionCliente(conexion_interrupt_kernel);
    return NULL;
}