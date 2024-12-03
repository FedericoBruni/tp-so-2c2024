#ifndef SYSCALLS_H
#define SYSCALLS_H
#include "utilidades.h"
#include "utils/utils.h"

void SYS_PROCESS_CREATE(char *archivo, int tamanio_memoria, int prioridad);
void PROCESS_EXIT(TCB *tcb);
void THREAD_CREATE(PCB *pcb,char* archivo_pseudocodigo, int prioridad);
void THREAD_EXIT(TCB *tcb);
void THREAD_JOIN(int tid);
void THREAD_CANCEL(int tid, int pid);
void MUTEX_CREATE(char* recurso);
void MUTEX_LOCK(char* recurso);
void MUTEX_UNLOCK(char* recurso);
void DUMP_MEMORY(int pid, int tid);
void IO(int tiempo);
void ejecucion_io(int tiempo);

#endif