#ifndef INTERFAZ_GENERICA_H_
#define INTERFAZ_GENERICA_H_
#include <utils/interfaces.h>
#include <commons/bitarray.h>

typedef struct {
    void * datos;
    int tamanio;
    int cantidadBloques;
} t_archivo_compac;


info_interfaz * crearTipoInterfaz(char* nombre, char* tipo_interfaz);
extern t_list * archivos_dial_fs;
extern FILE * bitmap;
extern FILE * bloques;
extern t_bitarray * bitmap_archivos;
extern void * espacio_bitmap;
extern char * ruta_bitmap;
extern char * ruta_bloques;
extern char * nombre_a_borrar;

int casteoStringInterfaz(char* tipo_interfaz);
int tamanioDireccionesFisicas(t_queue * direccionesFisicas, uint32_t cantidad_direccionesFisicas);
bool hayEspacioLibre(int cantidadBloques);
bool hayEspacioLibreContiguo(int bloque_inicial, int cantidad_bloques);
int calcularCantidadBloques(int tamanio);
void iniciarDialFS();
void compactar(char * nombre_archivo_pedido);
int obtenerBloqueLibre();
bool compararNombreArchivo(void * elemento);

#endif