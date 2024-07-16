#ifndef KERNEL_H_
#define KERNEL_H_

#include <commons/collections/queue.h>
#include <utils/conexiones.h>
#include <utils/conexiones.h>
#include <utils/procesos.h>
#include <semaphore.h>
#include <pthread.h>
#include <utils/interfaces.h>
#include <utils/hilos.h>

typedef struct {
    t_queue * cola;
    pthread_mutex_t * mutex_cola;
} cola_sincronizada;


typedef struct {
    char * nombre_interfaz;
    t_interfaz tipo_de_interfaz;
    sem_t * hay_procesos_en_interfaz;
    cola_sincronizada * cola_bloqueados;
} t_interfaz_kernel;

typedef struct {
    char * nombre_recurso;
    int cantidad_instancias;
    pthread_mutex_t * mutex_cantidad_instancias;
    cola_sincronizada * cola_procesos_bloqueados;
} t_recurso;

typedef enum {
    NEW,
    READY,
    BLOCKED,
    EXEC,
    EXIT
} estado_procesos;

typedef struct {
    uint32_t PID;
    estado_procesos estado;
    bool bloqueadoPorRecurso;
    char * elemento_bloqueador;
    bool esta_en_ready_plus;
    t_list * recursos_tomados;    
} t_estado_proceso;

typedef struct {
    char * nombre_recurso;
    int instancias_tomadas;
} recurso_tomado_proceso;

typedef struct {
    pcb * contexto;
    char * motivo_finalizacion;
} pcb_a_finalizar;

typedef struct{
    codigos_operacion operacion;
    t_queue * cola_parametros;
} peticion_memoria;

typedef struct{
    char * nombre_interfaz;
    pthread_mutex_t * mutex;
} t_mutex_eliminacion_interfaz;

cola_sincronizada * crearCola();
void agregarACola(cola_sincronizada * cola_elegida, void * valor);
void * sacarDeCola(cola_sincronizada * cola_elegida);
void eliminarColaSincronizada(cola_sincronizada * cola_elegida);
bool colaSincronizadaEstaVacia(cola_sincronizada * cola_elegida);
void eliminarColaSincronizadaYElementos(cola_sincronizada * cola_elegida, void (*funcion_eliminadora) (void *));

int obtenerEstadoPorPID(uint32_t pid);
void cambiarEstadoLista(uint32_t pid, estado_procesos estado_nuevo);
char * obtenerEstadoString(estado_procesos estado);
void avisarYCambiarEstado(uint32_t pid, estado_procesos estado_actual);

pcb_a_finalizar * crearPcbAFinalizar(pcb * proceso, char * motivo_fin);
void destruirPcbAFinalizar(pcb_a_finalizar * pcb_fin);
void agregarAColaExit(pcb * proceso, char * motivo_fin);

extern cola_sincronizada * cola_new;
extern cola_sincronizada * cola_ready;
extern cola_sincronizada * cola_ready_vrr;
extern cola_sincronizada * cola_exit;
extern cola_sincronizada * cola_memoria;
extern t_list * lista_interfaces;
extern sem_t * solicitaron_finalizar_proceso;
extern sem_t * no_surgio_otra_interrupcion;
extern sem_t * hay_peticion_en_cola;
extern pthread_t hilo_timer;
extern pthread_mutex_t *mutex_lista_interfaces;
extern entero_sincronizado * grado_multiprogramacion;
extern int pid_nuevo_proceso;
extern sem_t * hay_peticiones_memoria;
extern t_list * lista_recursos;
extern entero_sincronizado * diferencia_cambio_mp;
extern t_list * lista_estados;
extern sem_t * hay_que_parar_planificacion;
extern entero_sincronizado * pid_fin_usuario;
extern pthread_mutex_t  * mutex_pid_comparar;
extern pthread_mutex_t * mutex_nombre_recurso;
extern lista_sincronizada * lista_mutex_eliminacion_interfaz;
extern entero_sincronizado * pid_actual;
extern sem_t * grado_multiprogramacion_habilita;
extern sem_t * hay_procesos_cola_exit;
extern sem_t * hay_procesos_cola_new;
extern sem_t * hay_procesos_cola_ready;
extern entero_sincronizado * quantum_actual;

typedef struct {
    datos_conexion_h conexion;
    char * tipo_de_algoritmo;
} datos_planificacion;


typedef enum {
    FIFO,
    RR,
    VRR
} algoritmo;

extern algoritmo algoritmo_elegido;

algoritmo algoritmoPorString(char* algoritmo);

void avisarCambioEstado(uint32_t pid, const char * estado_anterior, const char * estado_actual);
void avisarBloqueo(uint32_t pid, const char * nombre_recurso_o_interfaz);

bool existeInterfazEnSistema(char * nombre_interfaz);
bool interfazPuedeRealizarOperacion(codigos_operacion codigo, char * nombre_interfaz);
t_interfaz_kernel * encontrarInterfazEnLista(char * nombre_interfaz);

void agregarInterfazALista(t_interfaz_kernel * interfaz_actual);

t_recurso * buscarRecurso(char * nombre_recurso);
void agregarAColaReadySegunAlgoritmoQuantum(pcb *proceso_bloqueado);

void cargarRecursos();

peticion_memoria * crearPeticionMemoria(codigos_operacion operacion);
void verSiPararPlanificacion();
void agregarAColaReadyComun(pcb *proceso);
bool verificarPIDSiPidioFinalizar(uint32_t pid);
void agregarAColaReadyComunOFinalizar(pcb * proceso);
void agregarAColaReadyPorQuantumOFinalizar(pcb * proceso_bloqueado);
void bloquearEnInterfazOFinalizar(pcb_cola_interfaz * pcb_a_cola, t_interfaz_kernel * interfaz_elegida, char * nombre_interfaz);
void hacerWaitOFinalizar(t_recurso * recurso, pcb * pcb_hace_wait);
void buscarYTratarDeEliminarEn(estado_procesos estado_actual, uint32_t pid);
void actualizarDatosRecursoEnProceso(uint32_t pid, char * nombre_recurso, int instancias_a_sumar);
void liberarRecursosYborrarEstadoSistema(uint32_t pid);
void agregarAListaEstados(t_estado_proceso *estado_nuevo);
void listarPIDsyEstadosActuales();
void eliminarInterfazDeSistema(t_interfaz_kernel * interfaz_actual);
void agregarMutexInterfazAlSistema(t_mutex_eliminacion_interfaz * nuevo_mutex);
t_mutex_eliminacion_interfaz * buscarMutexEliminacionLista(char * nombre_interfaz);


cola_sincronizada * cola_new;
cola_sincronizada * cola_ready;
cola_sincronizada * cola_ready_vrr;
cola_sincronizada * cola_exit;
cola_sincronizada * cola_memoria;
algoritmo algoritmo_elegido;
t_list * lista_interfaces;
sem_t * solicitaron_finalizar_proceso;
sem_t * no_surgio_otra_interrupcion;
sem_t * hay_peticion_en_cola;
pthread_t  hilo_timer;
pthread_mutex_t * mutex_lista_interfaces;
char * nombre_interfaz_a_comparar;
entero_sincronizado * grado_multiprogramacion;
int pid_nuevo_proceso;
sem_t * hay_peticiones_memoria;
t_list * lista_recursos;
entero_sincronizado * diferencia_cambio_mp;
t_list * lista_estados;
sem_t * hay_que_parar_planificacion;
entero_sincronizado * pid_fin_usuario;
lista_sincronizada * lista_mutex_eliminacion_interfaz;
entero_sincronizado * pid_actual;
sem_t * grado_multiprogramacion_habilita;
sem_t * hay_procesos_cola_exit;
sem_t * hay_procesos_cola_new;
sem_t * hay_procesos_cola_ready;
entero_sincronizado * quantum_actual;

bool interfazTieneNombre(void * interfaz);
bool compararNombre(void * recurso);
bool encontrarEstadoPid(void * elemento);
pcb * buscarYExtraerDeCola(cola_sincronizada * cola_sinc, uint32_t pid);
bool obtenerBloqueadorPorPID(uint32_t pid);
char * obtenerNombreBloqueadorPorPID(uint32_t pid);
void setearTipoBloqueadorYNombreEnSistema(uint32_t pid, bool es_recurso, char * nombre_bloqueador);
bool compararNombreRecursoSistema(void * recurso_sistema);
t_estado_proceso * encontrarEstadoPorPID(uint32_t pid);
bool encontrarPcbPID(void * proces);
pcb * buscarPCBAEliminar(estado_procesos estado_actual, uint32_t pid);
void liberarInstanciasRecurso(void * recurso_tom);
void listarPIDsEnReadyComun();
void listarPIDsEnReadyPlus();
void mostrarPIDsYEstados(void * elemento_estado);
void * transformarEstadoProcesoAString(void * elemento);
bool encontrarMutexEliminacionPorNombre(void * elemento);
void indicarSiEstaEnReadyComunONo(uint32_t pid, bool esReadyPlus);
#endif