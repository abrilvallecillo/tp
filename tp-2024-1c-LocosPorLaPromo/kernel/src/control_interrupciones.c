#include "control_interrupciones.h"
#include <utils/conexiones.h>
#include "kernel.h"
#include <utils/hilos.h>
#include "configuracion.h"
pthread_t manejo_interrupciones_quantum;

void enviarInterrupcionPorQuantum(int conexion_cpu_interrupt);
void enviarInterrupcionACPUConsola(int conexion_cpu_interrupt);
void * manejarInterrupcionesQuantum(void * conexion_interrupt);

void *interrupcionDeProcesos(void * sin_parametro) {
    int conexion_cpu_interrupt = crearConexionCliente(configuracion.IP_CPU, configuracion.PUERTO_CPU_INTERRUPT);
    int handshake_cpu_interrupt = enviarHandshake(conexion_cpu_interrupt, KERNEL);
    if(handshake_cpu_interrupt == -1) {
        fprintf(stderr, "Hubo problema al hacer el handshake con la cpu en la conexion interrupt");
        abort();
    }
    if(algoritmo_elegido == VRR || algoritmo_elegido == RR) {
        manejo_interrupciones_quantum = crearHilo(manejarInterrupcionesQuantum, &conexion_cpu_interrupt);
    }

    while(1) {
        sem_wait(solicitaron_finalizar_proceso);
        if(algoritmo_elegido != FIFO)
            pthread_cancel(hilo_timer);
        enviarInterrupcionACPUConsola(conexion_cpu_interrupt);
    }
    
    liberarConexionCliente(conexion_cpu_interrupt);
    return NULL;
}

void * manejarInterrupcionesQuantum(void * conexion_interrupt) {
    int * conexion_cpu_interrupt = (int *) conexion_interrupt;
    while(1) {
        sem_wait(no_surgio_otra_interrupcion);
        enviarInterrupcionPorQuantum(*conexion_cpu_interrupt);
    }

    return NULL;
}

void enviarInterrupcionPorQuantum(int conexion_cpu_interrupt) {
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) //Para el PID
    );

    agregarABufferUint32(buffer, leerEnteroSincronizado(pid_actual));
    enviarBufferProcesoConMotivo(buffer, INT_FIN_QUANTUM, conexion_cpu_interrupt);
}
void enviarInterrupcionACPUConsola(int conexion_cpu_interrupt) {
    t_buffer * buffer = crearBufferGeneral(
        sizeof(uint32_t) //Para el PID
    );

    agregarABufferUint32(buffer, leerEnteroSincronizado(pid_fin_usuario));
    enviarBufferProcesoConMotivo(buffer, INT_FIN_CONSOLA, conexion_cpu_interrupt);
}