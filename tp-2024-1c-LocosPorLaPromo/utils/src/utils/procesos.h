#ifndef PROCESOS_H_
#define PROCESOS_H_

#include<netdb.h>
#include <utils/conexiones.h>
#include <commons/collections/queue.h>

// -------------------------- REGISTROS DE CPU --------------------------
// PC : Program Counter, indica la próxima instrucción a ejecutar
// AX : Registro Numérico de propósito general
// BX : Registro Numérico de propósito general
// CX :  Registro Numérico de propósito general
// DX : Registro Numérico de propósito general
// EAX : Registro Numérico de propósito general
// EBX : Registro Numérico de propósito general
// ECX : Registro Numérico de propósito general
// EDX : Registro Numérico de propósito general
// SI : Contiene la dirección lógica de memoria de origen desde donde se va a copiar un string.
// DI : Contiene la dirección lógica de memoria de destino a donde se va a copiar un string.

typedef struct {
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
} registros_generales;

typedef struct {
    registros_generales generales;
    uint32_t PC;
    uint32_t SI;
    uint32_t DI;
}registros_CPU; //registros propios que va a usar la CPU

// -------------------------- PCB --------------------------

typedef struct {
    uint32_t PID; // Identificador del proceso (deberá ser un número entero, único en todo el sistema).
    uint32_t PC; // Número de la próxima instrucción a ejecutar.
    uint32_t Quantum; // Unidad de tiempo utilizada por el algoritmo de planificación VRR.
    registros_generales registros; // Estructura que contendrá los valores de los registros de uso general de la CPU.
} pcb;

typedef enum {
    CONTEXTO_A_EJECUTAR, //Se envía un contexto desde el kernel al cpu,
    INT_FIN_CONSOLA,
    INT_FIN_QUANTUM,
    P_EXIT,
    P_ERROR,
    P_IO_GEN_SLEEP,
    P_WAIT,
    P_SIGNAL,
    P_IO_STDIN_READ,
    P_IO_STDOUT_WRITE,
    P_IO_FS_CREATE,
    P_IO_FS_DELETE,
    P_IO_FS_TRUNCATE,
    P_IO_FS_WRITE,
    P_IO_FS_READ,
    CREAR_PROCESO,
    PROCESO_CREADO,
    INFO_INTERFAZ,
    PETICION_INSTRUCCION,
    ENVIAR_INSTRUCCION,
    NO_HAY_INTERRUPCION,
    CONTEXTO_VUELVE_INTERRUPCION,
    P_BORRAR_MEMORIA,
    OUT_OF_MEMORY,
    PEDIDO_MARCO,
    P_RESIZE,
    ESCRIBIR_MEMORIA,
    LEER_MEMORIA,
    PEDIR_TAM_PAG
} codigos_operacion;

typedef struct {
    pcb * contexto;
    uint32_t longitud_nombre_interfaz;
    char * nombre_interfaz;
    uint32_t unidades_de_trabajo;
} pcb_IOGENSLEEP;

typedef struct {
    pcb * contexto;
    codigos_operacion operacion;
    t_queue * cola_parametros;
} pcb_cola_interfaz;

typedef struct {
    uint32_t pid;
    uint32_t quantum;
    uint32_t longitud_direccion_codigo;
    char * direccion_codigo;
} inicializar_proceso;

typedef struct {
    uint32_t pid;
    uint32_t pc;
} peticion_instruccion;

typedef struct {
    uint32_t longitud;
    char * string_instruccion;
} t_instruccion;

typedef struct {
    pcb * contexto;
    uint32_t longitud_nombre_recurso;
    char * nombre_recurso;
} pcb_recurso;

typedef struct {
    pcb * contexto;
    uint32_t longitud_nombre_interfaz;
    char * nombre_interfaz;
    uint32_t cantidad_direccionesFisicas;
    t_queue * direccionesFisicas;
} pcb_IOSTD_IN_OUT;

typedef struct{
    uint32_t direccionFisica;
    uint8_t tamanioEnvio;
} t_direccionMemoria;

typedef struct{
    uint32_t pid;
    uint32_t numeroPagina;
} t_pedidoMarco;

typedef struct {
    t_direccionMemoria * direccion;
    uint32_t pid;
    void * valor;
} t_operacionMemEscribirUsuario;

typedef struct {
    t_direccionMemoria * direccion;
    uint32_t pid;
} t_operacionMemLeerUsuario;

typedef struct{
    uint32_t pid;
    uint32_t tamanio;
}peticion_resize;

typedef struct {
    pcb * contexto;
    uint32_t longitud_nombre_interfaz;
    char * nombre_interfaz;
    uint32_t longitud_nombre_archivo;
    char * nombre_archivo;
    uint32_t punteroArchivo;
    uint32_t cantidad_direccionesFisicas;
    t_queue * direccionesFisicas;
} pcb_FSWR;

typedef struct {
    pcb * contexto;
    uint32_t longitud_nombre_interfaz;
    char * nombre_interfaz;
    uint32_t longitud_nombre_archivo;
    char * nombre_archivo;
    uint32_t tamanio_nuevo_archivo;
} pcb_FSTRUN;

typedef struct {
    pcb * contexto;
    uint32_t longitud_nombre_interfaz;
    char * nombre_interfaz;
    uint32_t longitud_nombre_archivo;
    char * nombre_archivo;
} pcb_FSCREADEL;


t_buffer * serializarProceso(pcb * proceso_a_serializar);

pcb * deserializarProceso(t_buffer * buffer_paquete);

void enviarBufferProcesoConMotivo(t_buffer * buffer, codigos_operacion motivo, int un_socket); //Enviamos un proceso directamente con el motivo de desalojo

t_buffer * serializarProcesoIOGENSLEEP(pcb_IOGENSLEEP * proceso);

pcb_IOGENSLEEP * deserealizarProcesoIOGENSLEEP(t_buffer * buffer_paquete);

t_buffer * serializarInicializarProceso(inicializar_proceso * nueva_peticion);

inicializar_proceso * deserializarInicializarProceso(t_buffer * buffer);

t_buffer * serializarPeticionAInstruccion(peticion_instruccion * peticion);

peticion_instruccion * deserializarPeticionInstruccion(t_buffer * buffer_paquete);

t_buffer * serializarInstruccion(t_instruccion * instruccion);

t_instruccion * deserializarInstruccion(t_buffer * buffer_paquete);

t_buffer * serializarProcesoRecurso(pcb_recurso * pcb_con_recurso);

pcb_recurso * deserializarProcesoRecurso(t_buffer * buffer_paquete);

t_buffer * serializarProcesoIOSTDINOUT(pcb_IOSTD_IN_OUT * proceso);

pcb_IOSTD_IN_OUT * deserializarProcesoIOSTDINOUT(t_buffer *buffer_paquete);

uint32_t deserializarBorrarMemoriaP(t_buffer * buffer_paquete);

t_buffer * serializarPedidoMarco(int numeroPagina, int pid);

t_pedidoMarco * deserializarPedidoMarco(t_buffer * buffer_paquete);

t_buffer * serializarMarco(int numeroMarco);

int deserializarMarco(t_buffer *buffer_paquete);

t_operacionMemEscribirUsuario * crearOperacionMemEscribirUsuario(t_direccionMemoria * direccion, uint32_t pid, void * valor);

t_operacionMemLeerUsuario * crearOperacionMemLeerUsuario(t_direccionMemoria * direccion, uint32_t pid);

t_buffer * serializarOperacionMemEscribirUsuario(t_operacionMemEscribirUsuario * operacion);

t_operacionMemEscribirUsuario * deserializarOperacionMemEscribirUsuario(t_buffer * buffer_paquete);

t_buffer * serializarOperacionMemLeerUsuario(t_operacionMemLeerUsuario * operacion);

t_operacionMemLeerUsuario * deserializarOperacionMemLeerUsuario(t_buffer * buffer_paquete);

void hacerOperacionEscrituraMemoria(t_queue * direccionesFisicas, void * valor, uint32_t pid, int conexionMemoria);

void * hacerOperacionLecturaMemoria(t_queue * direccionesFisicas, void * valor, uint32_t pid, int conexionMemoria);

void agregarABufferDireccionesFisicas(t_buffer * buffer, t_queue * direccionesFisicas);

t_queue * leerDeBufferDireccionesFisicas(t_buffer * buffer, uint32_t cantidad_direccionesFisicas);

t_buffer * serializarPeticionResize(peticion_resize * peticion);

peticion_resize * deserializarPeticionResize(t_buffer * buffer_paquete);

t_buffer * serializarTamPagina(uint32_t tamanio_pagina);

int deserializarTamPagina(t_buffer * buffer_paquete);

t_buffer * serializarProcesoFSCREADEL(pcb_FSCREADEL * proceso_a_enviar);

pcb_FSCREADEL * deserializarProcesoFSCREADEL(t_buffer * buffer_paquete);

t_buffer * serializarProcesoFSTRUN(pcb_FSTRUN * proceso_a_enviar);

pcb_FSTRUN * deserializarProcesoFSTRUN(t_buffer * buffer_paquete);

t_buffer * serializarProcesoFSWR(pcb_FSWR * proceso_a_enviar);

pcb_FSWR * deserializarProcesoFSWR(t_buffer * buffer_paquete);
#endif