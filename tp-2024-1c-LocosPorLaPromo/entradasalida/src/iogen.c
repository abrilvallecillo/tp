#include "iogen.h"
#include <utils/interfaces.h>
#include <commons/string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include "configuracion.h"
#include <fcntl.h>
#include <utils/logger_concurrente.h>


t_list * archivos_dial_fs;
FILE * bloques;
FILE * bitmap;
t_bitarray * bitmap_archivos;
void * espacio_bitmap;
char * ruta_bitmap;
char * ruta_bloques;
char * nombre_a_borrar;
void limpiarBitmap();
bool archivoCreado(char * nombre);
void borrarArchivoCompac(void * elemento);


info_interfaz* crearTipoInterfaz(char* nombre, char* t_interfaz){
    info_interfaz* tipo_interfaz = malloc(sizeof(info_interfaz));
    tipo_interfaz->nombre = string_duplicate(nombre);
    tipo_interfaz->longitud_nombre = string_length(nombre) + 1;
    tipo_interfaz->tipo = casteoStringInterfaz(t_interfaz);
    return tipo_interfaz;
}

int casteoStringInterfaz(char* tipo_interfaz){
    if(!strcmp("GENERICA", tipo_interfaz)){ return GENERICA; }
    if(!strcmp("STDIN", tipo_interfaz)){ return STDIN; }
    if(!strcmp("STDOUT", tipo_interfaz)){ return STDOUT; }
    if(!strcmp("DIALFS", tipo_interfaz)){ return DIALFS; }
    return -1;
}

int tamanioDireccionesFisicas(t_queue * direccionesFisicas, uint32_t cantidad_direccionesFisicas){
    int tamanioTotal = 0;
    for (int i = 0; i < cantidad_direccionesFisicas; i++)
    {
        t_direccionMemoria * direccion = list_get(direccionesFisicas->elements, i);
        tamanioTotal += direccion->tamanioEnvio;
    }
    return tamanioTotal;
} 


void iniciarDialFS() {
    DIR * resultado_apertura = opendir(configuracion.PATH_BASE_DIALFS);
    if(resultado_apertura == NULL){
        mkdir(configuracion.PATH_BASE_DIALFS, S_IRWXG | S_IRWXO | S_IRWXU);
        resultado_apertura = opendir(configuracion.PATH_BASE_DIALFS);
    }
    struct dirent * archivo;
    archivo = readdir(resultado_apertura);
    while (archivo){
        if (archivoCreado(archivo->d_name)){
            char * nombre_archivo = string_duplicate(archivo->d_name);
            list_add(archivos_dial_fs, nombre_archivo);
        }
        archivo = readdir(resultado_apertura);
    }
    ruta_bitmap = string_duplicate(configuracion.PATH_BASE_DIALFS);
    string_append(&ruta_bitmap, "/bitmap.dat");
    bitmap = fopen(ruta_bitmap, "r+");
    espacio_bitmap = malloc(configuracion.BLOCK_COUNT/8);
    bitmap_archivos = bitarray_create_with_mode(espacio_bitmap, configuracion.BLOCK_COUNT/8, LSB_FIRST);
    limpiarBitmap();
    if(bitmap == NULL){
        bitmap = fopen(ruta_bitmap, "w+");
    } else {
        fseek(bitmap, 0, SEEK_SET);
        fread(bitmap_archivos->bitarray, configuracion.BLOCK_COUNT/8, 1, bitmap);
    }
    ruta_bloques = string_duplicate(configuracion.PATH_BASE_DIALFS);
    string_append(&ruta_bloques, "/bloques.dat");
    bloques = fopen(ruta_bloques, "r+");
    if(bloques == NULL){
        bloques = fopen(ruta_bloques, "w+");
        uint32_t offset = 0;
        void * valorVacio = malloc(configuracion.BLOCK_SIZE * configuracion.BLOCK_COUNT);
        while(offset < configuracion.BLOCK_SIZE * configuracion.BLOCK_COUNT) {
            uint8_t  auxiliar = 0;
            memcpy(valorVacio + offset, &auxiliar, sizeof(uint8_t));
            offset++;
        }
        fseek(bloques, 0, SEEK_SET);
        fwrite(valorVacio, configuracion.BLOCK_SIZE * configuracion.BLOCK_COUNT, 1, bloques);
        free(valorVacio);
    }
    closedir(resultado_apertura);
    fseek(bitmap, 0, SEEK_SET);
    fseek(bloques, 0, SEEK_SET);
}

bool archivoCreado(char * nombre){
    return (strcmp(".", nombre) && strcmp("..", nombre) && strcmp("bitmap.dat", nombre) && strcmp("bloques.dat", nombre));
}

bool hayEspacioLibre(int cantidadBloques){
    int bloquesLibres = 0;
    for(int i = 0; i < bitarray_get_max_bit(bitmap_archivos); i++) {
        bloquesLibres += bitarray_test_bit(bitmap_archivos, i);
    }
    return bloquesLibres >= cantidadBloques;
}

bool hayEspacioLibreContiguo(int bloque_inicial, int cantidad_bloques) {
    int bloquesLibres = 0;
    for(int i = bloque_inicial; i < bitarray_get_max_bit(bitmap_archivos); i++) {
        if(bitarray_test_bit(bitmap_archivos, i)) {
            bloquesLibres++;
            if(bloquesLibres == cantidad_bloques)
                break;
        } else 
            break;
    }
    return bloquesLibres == cantidad_bloques;
}

void compactar(char * nombre_archivo_pedido){
    usleep(configuracion.RETRASO_COMPACTACION * 1000);
    nombre_a_borrar = string_duplicate(nombre_archivo_pedido);
    char * nombre_archivo = list_find(archivos_dial_fs, compararNombreArchivo);
    free(nombre_a_borrar);
    list_remove_element(archivos_dial_fs, nombre_archivo);
    list_add(archivos_dial_fs, nombre_archivo);
    t_list * lista_elementos_a_acomodar = list_create();
    for(int i = 0; i < list_size(archivos_dial_fs); i++) {
        char * ruta_archivo = string_duplicate(configuracion.PATH_BASE_DIALFS);
        string_append(&ruta_archivo, "/");
        string_append(&ruta_archivo, list_get(archivos_dial_fs, i));
        t_config * fcb_archivo = config_create(ruta_archivo);
        t_archivo_compac * archivo_comp = malloc(sizeof(t_archivo_compac));
        int bloque_inicial = config_get_int_value(fcb_archivo, "BLOQUE_INICIAL");
        archivo_comp->tamanio = config_get_int_value(fcb_archivo, "TAMANIO_ARCHIVO");
        archivo_comp->datos = malloc(archivo_comp->tamanio);
        archivo_comp->cantidadBloques = calcularCantidadBloques(archivo_comp->tamanio);

        fseek(bloques, bloque_inicial*configuracion.BLOCK_SIZE, SEEK_SET);
        fread(archivo_comp->datos, archivo_comp->tamanio, 1, bloques);
        list_add(lista_elementos_a_acomodar, archivo_comp);
        config_destroy(fcb_archivo);
        free(ruta_archivo);
    }
    limpiarBitmap();
    int offset = 0;
    for(int i = 0; i < list_size(archivos_dial_fs); i++) {
        t_archivo_compac * archivo_comp = list_get(lista_elementos_a_acomodar, i);
        fseek(bloques, offset*configuracion.BLOCK_SIZE, SEEK_SET);
        fwrite(archivo_comp->datos, archivo_comp->tamanio, 1, bloques);
        
        for(int j = offset; j < archivo_comp->cantidadBloques + offset -1; j++) {
            bitarray_clean_bit(bitmap_archivos, j);
        }

        char * ruta_archivo = string_duplicate(configuracion.PATH_BASE_DIALFS);
        string_append(&ruta_archivo, "/");
        string_append(&ruta_archivo, list_get(archivos_dial_fs, i));
        t_config * fcb_archivo = config_create(ruta_archivo);
        char * string_bloque_inicial = string_itoa(offset);
        config_set_value(fcb_archivo, "BLOQUE_INICIAL", string_bloque_inicial);
        free(string_bloque_inicial);
        config_save(fcb_archivo);
        config_destroy(fcb_archivo);
        free(ruta_archivo);
        offset += archivo_comp->cantidadBloques;
    }
    list_destroy_and_destroy_elements(lista_elementos_a_acomodar, borrarArchivoCompac);
}

void borrarArchivoCompac(void * elemento) {
    t_archivo_compac * archivo_comp = (t_archivo_compac *) elemento;
    free(archivo_comp->datos);
    free(archivo_comp);
}
int calcularCantidadBloques(int tamanio) {
    int cantidad_parcial = (int) (tamanio/configuracion.BLOCK_SIZE);
    if(tamanio%configuracion.BLOCK_SIZE || tamanio == 0)
        cantidad_parcial++;
    return cantidad_parcial;
}

void limpiarBitmap() {
    for(int i=0; i < bitarray_get_max_bit(bitmap_archivos); i++) {
        bitarray_set_bit(bitmap_archivos, i);
    } 
}

int obtenerBloqueLibre(){
    for(int i=0; i < bitarray_get_max_bit(bitmap_archivos); i++) {
        if(bitarray_test_bit(bitmap_archivos, i)) {
            bitarray_clean_bit(bitmap_archivos, i);
            return i;
        }
    }
    return -1;
}

bool compararNombreArchivo(void * elemento) {
    char * nombre = (char *) elemento;
    return !strcmp(nombre, nombre_a_borrar); 
}