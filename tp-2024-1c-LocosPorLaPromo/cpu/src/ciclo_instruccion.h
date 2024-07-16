#ifndef CICLO_INSTRUCCION_H_
#define CICLO_INSTRUCCION_H_

#include <netdb.h>
#include "cpu.h"

typedef enum{
    SET,
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,
    WAIT,
    SIGNAL,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT
} operaciones_cpu;

typedef struct {
    int operacion;
    t_queue * parametros;
}instruccion_decodificada;

void cicloDeInstruccion(uint32_t pid, int volvio_de_interrupcion);
char * fetch(uint32_t pid, uint32_t pc);
int casteoStringInstruccion(char* instruccion);
registro_casteado * casteoRegistro(char * operando_registro);
instruccion_decodificada * decode(char * proxima_instruccion);
void execute(instruccion_decodificada * instruccion);

int casteoStringInstruccion(char* instruccion);
registro_casteado * casteoRegistro(char * operando_registro);

#endif
