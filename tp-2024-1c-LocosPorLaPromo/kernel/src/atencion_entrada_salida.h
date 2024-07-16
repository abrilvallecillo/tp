#ifndef ATENCION_IO_H_
#define ATENCION_IO_H_

void * atenderIO(void * socket_cliente_io);
t_interfaz_kernel * recibirTipoInterfaz(int conexion_cliente_io);
t_interfaz_kernel * deserializarInformacionInterfaz(t_buffer * buffer);
void enviarOperacionAInterfaz(pcb_cola_interfaz * pcb_a_trabajar, int conexion_cliente_io);
int recibirOperacionAInterfaz(int conexion_cliente_io);
void eliminarInterfaz(t_interfaz_kernel * interfaz_actual);
void mandarAFinalizarLosProcesos(void * elemento);
void matarProcesoInterfaz(pcb_cola_interfaz *proceso_a_matar);
#endif