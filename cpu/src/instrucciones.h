#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H
#include "utils/utils.h"
#include "MMU.h"
#include "main.h"

void SET(char *registro, uint32_t valor);
void read_mem(char* registroDatos, char* registroDireccion);
void write_mem(char* registroDatos, char* registroDireccion);
void SUM(char *registro_destino, char *registro_origen);
void SUB(char *registro_destino, char *registro_origen);
void JNZ(char *registro, uint32_t instruccion);
void LOG(char *registro);
void DUMP_MEMORY(int pid, int tid);
void io(int tiempo);
void PROCESS_CREATE(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo);
void THREAD_CREATE (char* archivo_pseudocodigo, int prioridad);
char* THREAD_JOIN (int tid);
void THREAD_CANCEL (int tid, int pid);
void MUTEX_CREATE (char *recurso);
char* MUTEX_LOCK (char* recurso);
void MUTEX_UNLOCK (char* recurso);
void THREAD_EXIT();
void PROCESS_EXIT();
#endif /*INSTRUCCIONES_H*/