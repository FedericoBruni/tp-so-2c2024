#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H
#include "utils/utils.h"
#include "main.h"

void SET(char *registro, uint32_t valor);
void READ_MEM(char* registroDatos, char* registroDireccion);
void SUM(char *registro_destino, char *registro_origen);
void SUB(char *registro_destino, char *registro_origen);
void JNZ(char *registro, uint32_t instruccion);
void LOG(char *registro);
void DUMP_MEMORY(int pid, int tid);
void IO (int tiempo);
void PROCESS_CREATE(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo);
void THREAD_CREATE (char* archivo_pseudocodigo, int prioridad);
void THREAD_JOIN (int tid);
void THREAD_CANCEL (int tid, int pid);
void MUTEX_CREATE (char *recurso);
void MUTEX_LOCK (char* recurso);
void MUTEX_UNLOCK (char* recurso);
void THREAD_EXIT(int tid, int pid);
void PROCESS_EXIT(int pid);
#endif /*INSTRUCCIONES_H*/