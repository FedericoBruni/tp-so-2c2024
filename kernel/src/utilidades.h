#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include "utils/utils.h"
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "planificador_largo_plazo.h"
#include "pthread.h"
#include "syscalls.h"
#include "mensajeria.h"
#include "pcb.h"
#include <semaphore.h>

void iniciar_kernel(void);

int conectarse_a_cpu_interrupt(void);
int conectarse_a_cpu_dispatch(void);
int conectarse_a_memoria(void);
void terminar_ejecucion(int dispatch, int memoria, int interrupt);
void mover_tcbs_exit(PCB *pcb);
void encolar(t_queue* cola, void* elemento, pthread_mutex_t mutex);
void* desencolar(t_queue* cola, pthread_mutex_t mutex);
void iniciar_semaforos(void);
void inicializar_mutex(pthread_mutex_t* mutex, char* nombre);
void inicializar_semaforo(sem_t* semaforo, char* nombre, int valor);

#endif // UTILIDADES_H_