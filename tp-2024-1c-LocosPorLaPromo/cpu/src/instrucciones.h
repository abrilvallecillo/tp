#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include "cpu.h"
void hacerOperacionEscrituraMemoriaCPU(t_queue * direccionesFisicas, void * valor, uint32_t pid, int reg8_reg32_cpystr);
void * hacerOperacionLecturaMemoriaCPU(t_queue * direccionesFisicas, void * valor, uint32_t pid, int reg8_reg32_cpystr);

void set(registro_casteado *registro, int valor);
void mov_in(registro_casteado *registroDatos, registro_casteado *registroDireccion) ;
void mov_out(registro_casteado *registroDireccion, registro_casteado *registroDatos);
void sum(registro_casteado *registroDestino, registro_casteado *registroOrigen);
void sub(registro_casteado *registroOrigen, registro_casteado *registroDestino);
void jnz(registro_casteado *registro, int nroInstruccion);
void resize(int tamanio);
void copy_string(int tamanio);
void wait(char * recurso);
void signal(char * recurso);
void io_gen_sleep(char* interfaz, int  *unidadesDeTrabajo);
void io_stdin_read(char* interfaz, registro_casteado *registroDireccion, registro_casteado *registroTamanio);
void io_stdout_write(char* interfaz, registro_casteado *registroDireccion, registro_casteado *registroTamanio);
void io_fs_create(char* interfaz, char* nombreArchivo);
void io_fs_delete(char* interfaz, char* nombreArchivo);
void io_fs_truncate(char* interfaz, char* nombreArchiv, registro_casteado *registroTamanio);
void io_fs_write(char* interfaz, char* nombreArchiv, registro_casteado *registroDireccion, registro_casteado *registroTamanio, registro_casteado *registropunteroArchivo);
void io_fs_read(char* interfaz, char* nombreArchiv, registro_casteado *registroDireccion, registro_casteado *registroTamanio, registro_casteado *registropunteroArchivo);
void exitProgram();
#endif