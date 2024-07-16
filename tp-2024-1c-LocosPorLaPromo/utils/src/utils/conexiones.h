#ifndef UTILS_CONEXION_H_
#define UTILS_CONEXION_H_

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<netdb.h>
#include <commons/log.h>
#include<commons/collections/list.h>

typedef struct {
    uint32_t size; // Tamaño del payload
    uint32_t offset; // Desplazamiento dentro del payload
    void* stream; // Payload
} t_buffer;

typedef enum{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct {
    uint8_t codigoOperacion;
    t_buffer* buffer;
} t_paquete;

/*Datos comunes para realizar una conexión desde un hilo nuevo*/
typedef struct {
    char * puerto;
    char * ip;
} datos_conexion_h;

enum handshake {
    KERNEL,
    CPU,
    MEMORIA,
    ENTRADASALIDA
};

enum ResultadosHandshake {
    ERROR_HS = -1,
    OK_HS
};


int crearConexionCliente(char *ip, char* puerto);

int crearConexionServidor(char * puerto);

int esperarCliente(int socket_servidor);

int recibir_operacion(int socket_cliente);

int enviarHandshake(int unSocket, int moduloDeOrigen); //Enviamos un handshake en la conexión unSocket desde el modulo móduloDeOrigen, que es uno de los valores del enum del handshake

int recibirHandshake(int unSocketCliente); //Es para recibir un handshake de un cliente

void enviar_mensaje(char* mensaje, int socket_cliente);

t_paquete* crear_paquete(void);

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);

void enviar_paquete(t_paquete* paquete, int socket_cliente);

void eliminar_paquete(t_paquete* paquete);

void* recibir_buffer(int*, int);

t_list* recibir_paquete(int);

void recibir_mensaje(int);

int recibir_operacion(int);

void enviarBufferPorPaquete(t_buffer *buffer, uint8_t codigo_de_operacion, int un_socket);

void agregarABufferUint32(t_buffer * buffer, uint32_t valor_a_guardar);

void agregarABufferUint8(t_buffer * buffer, uint8_t valor_a_guardar);

t_buffer * crearBufferGeneral(uint32_t tamanio);

t_paquete *recibirPaqueteGeneral(int un_socket);

uint32_t leerDeBufferUint32(t_buffer * buffer);

uint8_t leerDeBufferUint8(t_buffer * buffer);

char * leerDeBufferString(t_buffer * buffer, uint32_t tamanio_string);

void agregarABufferString(t_buffer * buffer, char * string, uint32_t tamanio_string);

void agregarABufferVoidP(t_buffer * buffer, void * valor, uint32_t tamanio_valor);

void * leerDeBufferVoidP(t_buffer * buffer, uint32_t tamanio);

void liberarConexionCliente(int socket_cliente);

void marcarSocketServidorComoReusable(int socket_servidor);
#endif