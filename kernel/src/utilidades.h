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

void iniciar_kernel(void);

int conectarse_a_cpu_interrupt(void);
int conectarse_a_cpu_dispatch(void);
int conectarse_a_memoria(void);
void terminar_ejecucion(int dispatch, int memoria, int interrupt);
PCB *crear_pcb(char *archivo, int tamanio_memoria, int prioridad_main);
TCB *crear_tcb(PCB *pcb, int prioridad);
void liberar_tcb(void *ptr_tcb);
void liberar_registros(REGISTROS *registros);
void liberar_pcb(void *ptr_pcb);
void imprimir_pcb(PCB *pcb);
void imprimir_registros(REGISTROS *registros);
void imprimir_lista_ids(t_list *tids);
void imprimir_hilos(t_list *threads);
void agregar_hilo(TCB *tcb, PCB *pcb);
void cambiar_estado(PCB *pcb, STATUS estado);
void inicializar_registros(REGISTROS *registros);

#endif // UTILIDADES_H_