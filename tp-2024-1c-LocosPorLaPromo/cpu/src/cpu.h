#ifndef CPU_H_
#define CPU_H_
#include <utils/procesos.h>
#include <utils/hilos.h>

typedef struct {
    bool es32bits;
    void * registro;
} registro_casteado;

extern bool incrementaPC;
extern registros_CPU registrosPropios;
extern int conexion_dispatch_kernel;
extern entero_sincronizado * estado_interrupcion;
extern t_queue * cola_parametros;
extern int conexion_memoria;
extern int interrupcion_proceso;
extern entero_sincronizado * pid_a_finalizar;
extern uint32_t pid_actual;
extern int tamanioPagina;
extern entero_sincronizado * consumirFinQ;

pcb_IOGENSLEEP * crearPCB_IOGENSLEEP(pcb * proceso_elegido);
pcb_recurso * crearPCBRecurso(pcb * proceso_elegido);
pcb_IOSTD_IN_OUT * crearPCB_STDINOUT(pcb * proceso_elegido);
pcb_FSCREADEL * crearPCB_FSCREADEL(pcb * proceso_elegido);
pcb_FSTRUN * crearPCB_FSTRUN(pcb * proceso_elegido);
pcb_FSWR * crearPCB_FSWR(pcb * proceso_elegido);

void cargarRegistrosEnCPU(pcb * proceso_elegido);
void cargarRegistrosEnPCB(pcb * proceso_ejecutado);

t_queue * traducirMemoria(int direccionLogica, int tamanioDato);
registro_casteado * crearRegistroCasteado(bool es32bits, void * direccion_registro);
void modificarValorRegistroCasteado(registro_casteado * registro_cast, int valor);
int leerValorRegistroCasteado(registro_casteado * registro_cast);
int tamanioRegistroCasteado(registro_casteado * registro_cast);

bool esInterrupcionSincronica(int interrupcion);

int minInt(int entero1, int entero2);
t_direccionMemoria * direccionLogicaAFisica(int numeroPagina, int tamanioAEnviar, int pid);
void solicitarMarcoAMemoria(int numeroPagina, int pid);
int recibirMarcoDeMemoria();
#endif
