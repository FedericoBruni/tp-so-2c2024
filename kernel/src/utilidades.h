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
#include "planificador_corto_plazo.h"
#include "pthread.h"
#include "syscalls.h"
#include "mensajeria.h"
#include "pcb.h"
#include <semaphore.h>
#include <commons/string.h>
#include <commons/temporal.h>

typedef struct{
    int prioridad;
    t_queue *cola_prioridad;
    int quantum;
    pthread_mutex_t mutex;
} COLA_PRIORIDAD;

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
bool comparar_prioridad(void *a, void *b);
COLA_PRIORIDAD* crear_multinivel(TCB* tcb);
void encolar_multinivel(COLA_PRIORIDAD *cola, TCB *tcb);
void* desencolar_multinivel(COLA_PRIORIDAD *cola);
COLA_PRIORIDAD* existe_cola_con_prioridad(int prioridad);
COLA_PRIORIDAD* obtener_cola_con_mayor_prioridad();
void bloquear_hilo_syscall(TCB *tcb,int tid);
void desbloquear_bloqueados_por_hilo(int tidBloqueante);
void replanificar(TCB *tcb);
MUTEX *existe_mutex(char* recurso);
void asignar_a_hilo_mutex(MUTEX *mutex, TCB *tcb);
void desbloquear_hilo_mutex(MUTEX *mutex);
void bloquear_hilo_mutex(t_list* cola_bloqueados, TCB* tcb);
void imprimir_cola(t_queue *cola, pthread_mutex_t mutex);
bool buscar_en_cola(t_queue *cola, pthread_mutex_t mutex, int pid);
bool buscar_en_cola_prioridad(COLA_PRIORIDAD *cola_prioridad, int pid);
void ordenar_cola(t_queue *cola, pthread_mutex_t mutex);
#endif // UTILIDADES_H_