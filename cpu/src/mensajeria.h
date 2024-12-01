#ifndef MENSAJERIA_H
#define MENSAJERIA_H
#include "utils/utils.h"
#include "utilidades.h"
#include "main.h"

void actualizar_contexto(int fd_memoria);
void crear_proceso(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo);
void crear_hilo(char* archivo_pseudocodigo, int prioridad);
void thread_join(int tid); // tid del hilo a desbloquear
void thread_cancel(int tid, int pid);
void mutex_create(char* recurso);
void mutex_lock(char* recurso);
void mutex_unlock(char* recurso);
void thread_exit(int tid, int pid);
void process_exit(int pid);
void enviar_dump_memory(int pid, int tid);
void enviar_io(int tiempo);

#endif /*MENSAJERIA_H*/