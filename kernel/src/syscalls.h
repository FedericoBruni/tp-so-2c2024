#ifndef SYSCALLS_H
#define SYSCALLS_H
#include "utilidades.h"
#include "utils/utils.h"

void PROCESS_CREATE(char *archivo, int tamanio_memoria, int prioridad);
void PROCESS_EXIT(TCB *tcb);

#endif