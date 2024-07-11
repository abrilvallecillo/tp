#include "conexiones.h"
#include<sys/socket.h>
#include <commons/string.h>
#include<signal.h>
#include <string.h>

uint32_t noEsUnModulo(uint32_t handshake); //Devuelve si el handshake viene de un modulo

int crearConexionCliente(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);
	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	/* Acá lo que hacemos es un loop para que al fallar la conexión porque el servidor no esté conectado, siga intentándolo*/
	int tieneError;
	do {
		tieneError = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);
	} while(tieneError);
	

	freeaddrinfo(server_info);

	return socket_cliente;
}

int crearConexionServidor(char * puerto){
    int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	// Lo marcamos como reusables
	marcarSocketServidorComoReusable(socket_servidor);
	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	return socket_servidor;
}

int esperarCliente(int socket_servidor){

	// Aceptamos un nuevo cliente
	int socket_cliente;
	
	socket_cliente = accept(socket_servidor, NULL, NULL);

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void enviarBufferPorPaquete(t_buffer *buffer, uint8_t codigo_de_operacion, int un_socket) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigoOperacion = codigo_de_operacion; // Podemos usar una constante por operación
	paquete->buffer = buffer; // Nuestro buffer de antes.
	
	// Armamos el stream a enviar
	void* aEnviar = malloc(buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
	int offset = 0;

	memcpy(aEnviar + offset, &(paquete->codigoOperacion), sizeof(uint8_t));

	offset += sizeof(uint8_t);
	memcpy(aEnviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(aEnviar + offset, paquete->buffer->stream, paquete->buffer->size);

	// Por último enviamos
	send(un_socket, aEnviar, buffer->size + sizeof(uint8_t) + sizeof(uint32_t), MSG_NOSIGNAL);

	// No nos olvidamos de liberar la memoria que ya no usaremos
	free(aEnviar);
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

int enviarHandshake(int unSocket, int moduloDeOrigen) { //Cuando un módulo (moduloDeOrigen) quiere hacer un handshake con un modulo de determinada conexion por unSocket

	int32_t handshake = (int32_t) moduloDeOrigen;
	int32_t resultado;

	send(unSocket, &handshake, sizeof(int32_t), 0);
	recv(unSocket, &resultado, sizeof(int32_t), MSG_WAITALL);

	return resultado;
}

int recibirHandshake(int unSocketCliente) {

	int32_t handshake;
	int32_t resultado;
	recv(unSocketCliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	resultado = noEsUnModulo(handshake);

	send(unSocketCliente, &resultado, sizeof(uint32_t), 0);
	return resultado;
}

uint32_t noEsUnModulo(uint32_t handshake) {
	return -1 * ((handshake > ENTRADASALIDA) || (handshake < KERNEL));
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigoOperacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigoOperacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	if(paquete->buffer->size){
		free(paquete->buffer->stream);
	}
	free(paquete->buffer);
	free(paquete);
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	free(buffer);
}

t_buffer * crearBufferGeneral(uint32_t tamanio) {
	t_buffer * buffer = malloc(sizeof(t_buffer));
	buffer->offset = 0;
	buffer->size = tamanio;
	buffer->stream = malloc(buffer->size);
	return buffer;
}

void agregarABufferUint32(t_buffer * buffer, uint32_t valor_a_guardar) {
	uint32_t valor = valor_a_guardar;
	memcpy(buffer->stream + buffer->offset, &valor, sizeof(uint32_t));
	buffer->offset += sizeof(uint32_t);
}

void agregarABufferUint8(t_buffer * buffer, uint8_t valor_a_guardar) {
	uint8_t valor = valor_a_guardar;
	memcpy(buffer->stream + buffer->offset, &valor, sizeof(uint8_t));
	buffer->offset += sizeof(uint8_t);
}

void agregarABufferString(t_buffer * buffer, char * string, uint32_t tamanio_string) {
	agregarABufferUint32(buffer, tamanio_string);
	memcpy(buffer->stream + buffer->offset, string, tamanio_string);
	buffer->offset += tamanio_string;
}

void agregarABufferVoidP(t_buffer * buffer, void * valor, uint32_t tamanio_valor) {
	memcpy(buffer->stream + buffer->offset, valor, tamanio_valor);
	buffer->offset += tamanio_valor;
}

t_paquete *recibirPaqueteGeneral(int un_socket) {
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	// Primero recibimos el codigo de operacion
	int tamanio_recibido = recv(un_socket, &(paquete->codigoOperacion), sizeof(uint8_t), 0);
	if(tamanio_recibido < sizeof(uint8_t)) {
		free(paquete->buffer);
		free(paquete);
		return NULL;
	}
	// Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
	recv(un_socket, &(paquete->buffer->size), sizeof(uint32_t), 0);
	if(paquete->buffer->size > 0) {
		paquete->buffer->stream = malloc(paquete->buffer->size);
		recv(un_socket, paquete->buffer->stream, paquete->buffer->size, 0);
	}
	paquete->buffer->offset = 0;

	return paquete;
}

uint32_t leerDeBufferUint32(t_buffer * buffer) {
	uint32_t valor;
	memcpy(&valor, buffer->stream + buffer->offset, sizeof(uint32_t));
	buffer->offset += sizeof(uint32_t);
	return valor;
}

uint8_t leerDeBufferUint8(t_buffer * buffer) {
	uint8_t valor;
	memcpy(&valor, buffer->stream + buffer->offset, sizeof(uint8_t));
	buffer->offset += sizeof(uint8_t);
	return valor;
}

void * leerDeBufferVoidP(t_buffer * buffer, uint32_t tamanio) {
	void *valor = malloc(tamanio);
	memcpy(valor, buffer->stream + buffer->offset, tamanio); 
	buffer->offset += tamanio;
	return valor;
}

char * leerDeBufferString(t_buffer * buffer, uint32_t tamanio_string) {
	char * string = malloc(tamanio_string);
	memcpy(string, buffer->stream + buffer->offset, tamanio_string);
	buffer->offset += tamanio_string; 
	return string;
}

void liberarConexionCliente(int socket_cliente) {
	close(socket_cliente);
}

void marcarSocketServidorComoReusable(int socket_servidor) {
	const int habilita = 1;
    if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &habilita, sizeof(int)) < 0) {
        fprintf(stderr, "Error al crear el socket");
        abort();
    }
}