#ifndef SYSCALLS_H
#define SYSCALLS_H
#include "utilidades.h"
#include "utils/utils.h"

void PROCESS_CREATE(char *archivo, int tamanio_memoria, int prioridad);
void PROCESS_EXIT(TCB *tcb);
void THREAD_CREATE(char* archivo_pseudocodigo, int prioridad);
void THREAD_EXIT(TCB *tcb);
void THREAD_CANCEL(TCB *tcb);
void MUTEX_CREATE(char* recurso);
void MUTEX_LOCK(char* recurso);

#endif