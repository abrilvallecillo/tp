#include "tlb.h"
#include <commons/temporal.h>
#include "configuracion.h"
#include <stdlib.h>

// -------------------------- TLB --------------------------

int buscarMarcoEnTLB(int numeroPagina, int pid){
    paginaAComparar = numeroPagina;
    pidAComparar = pid;
   
    direccionTLB * direccionEncontrada = list_find(TLB, buscarDireccion);
    
    if(direccionEncontrada) {
        if(algoritmo == LRU) {

            list_remove_element(TLB, direccionEncontrada);
            list_add(TLB, direccionEncontrada);

        } return direccionEncontrada->numeroMarco;
    } return -1;
}

bool buscarDireccion(void* filaTLB){
    direccionTLB * direccion = (direccionTLB *) filaTLB;
    bool TLB_Hit = pidAComparar == direccion->pid && paginaAComparar == direccion->numeroPagina;
    return (TLB_Hit);
}

void agregarDireccionATLB(int pid, int numeroPagina, int numeroMarco){
    if(configuracion.CANTIDAD_ENTRADAS_TLB == 0) {return;}
   
    if(list_size(TLB) == configuracion.CANTIDAD_ENTRADAS_TLB){
        direccionTLB * direccionASustituir = elegirCandidatoSegunAlgoritmo();
       
        direccionASustituir->pid = pid;
        direccionASustituir->numeroPagina = numeroPagina;
        direccionASustituir->numeroMarco = numeroMarco;
       
       list_remove_element(TLB, direccionASustituir);
       list_add(TLB, direccionASustituir);  

    } else{
        direccionTLB* direccionNueva = malloc(sizeof(direccionTLB));
        
        direccionNueva->pid = pid;
        direccionNueva->numeroPagina = numeroPagina;
        direccionNueva->numeroMarco = numeroMarco;

        list_add(TLB, direccionNueva);
    }
}

direccionTLB * elegirCandidatoSegunAlgoritmo(){
    direccionTLB * candidato; 

    switch(algoritmo){
        case FIFO:
            candidato = (direccionTLB*) list_get(TLB, 0);
            break;
        case LRU:
            candidato = (direccionTLB*) list_get(TLB, 0);
            break;
        default: fprintf(stderr, "Algoritmo TLB no reconocido.");
    } return candidato;
}