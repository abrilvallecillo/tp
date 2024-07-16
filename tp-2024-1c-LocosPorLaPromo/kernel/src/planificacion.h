#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

void *planificarProcesos(void * sin_parametro);

pcb * obtenerProcesoPorFIFO(void);
pcb * obtenerProcesoPorRR(void);
pcb * obtenerProcesoPorVRR(void);
pcb * seleccionarProcesoPorAlgoritmo(void);
void enviarContexto(pcb * proceso_a_enviar, int conexion_cpu_dispatch);
void operarSegunMotivoDesalojo(t_paquete * proceso_desalojado_paq, int conexion_cpu_dispatch);
void * pasarProcesosEstadoAExit(void * sin_parametro);
void * pasarProcesosNewAReady(void * sin_parametro);
void * contabilizarTiempoEjecucion(void * quantum_proceso);
void eliminarProceso(pcb_a_finalizar * pcb_fin);
void haceWaitOSignal(pcb_recurso * pcb, codigos_operacion waitOSignal, int conexion_cpu_dispatch);
void aplicarWait(pcb * pcb_hace_wait, t_recurso * recurso, int conexion_cpu_dispatch);
void aplicarSignal(pcb * pcb_hace_signal, t_recurso * recurso, int conexion_cpu_dispatch); 
void actualizarQuantum(pcb * pcb_a_actualizar);
void hacerOperacionIO(t_paquete * paquete_io);
char * deserializacionOperacionesIO(pcb_cola_interfaz * pcb_a_cola, t_paquete * paquete_io);
void cargarPcbColaInterfazIOGENSLEEP(pcb_cola_interfaz * pcb_a_cola, pcb_IOGENSLEEP * pcb_gensleep);
void cargarPcbColaInterfazIOSTDInOut(pcb_cola_interfaz * pcb_a_cola, pcb_IOSTD_IN_OUT * pcb_in_out);
void cargarPcbColaInterfazFSCREADEL(pcb_cola_interfaz * pcb_a_cola, pcb_FSCREADEL * pcb_CREADEL);
void cargarPcbColaInterfazFSTRUN(pcb_cola_interfaz * pcb_a_cola, pcb_FSTRUN * pcb_TRUN);
void cargarPcbColaInterfazFSWR(pcb_cola_interfaz * pcb_a_cola, pcb_FSWR * pcb_WR);
void hacerMensajeFinQuantum(uint32_t pid);
void hacerMensajeFinalizacionProceso(uint32_t pid, char *motivo_finalizacion);
bool hacerSignalSegunCambioMP();
uint32_t maxI(int a, int b);

pthread_t pasar_procesos_new_ready;
pthread_t pasar_procesos_estado_exit;
pthread_t hacer_quantum;



t_temporal * cronometro_vrr;
bool seHizoSyscallNoBloqueante;
#endif