#ifndef TLB_H_
#define TLB_H_
#include <commons/collections/list.h>

extern t_list * TLB;
t_list * TLB;
int paginaAComparar;
int pidAComparar;
ALGORITMO_TLB algoritmo;

typedef enum{
    FIFO,
    LRU
}ALGORITMO_TLB;

// Estructura TLB: [ pid | página | marco ]
typedef struct{
    int pid;
    int numeroPagina;
    int numeroMarco;
}direccionTLB;

extern ALGORITMO_TLB algoritmo;

int buscarMarcoEnTLB(int numeroPagina, int pid);

bool buscarDireccion(void* filaTLB);

void agregarDireccionATLB(int pid, int numeroPagina, int marcoMemoria);

direccionTLB * elegirCandidatoSegunAlgoritmo();

#endif