#ifndef PCB_H
#define PCB_H
#include "utilidades.h"
#include "utils/utils.h"

PCB *crear_pcb(char *archivo_pseudocodigo, int tamanio_memoria, int prioridad_main);
TCB *crear_tcb(PCB *pcb, int prioridad, char* archivo_pseudocodigo);
void liberar_tcb(void *ptr_tcb);
void liberar_registros(REGISTROS *registros);
void liberar_pcb(void *ptr_pcb);
void imprimir_pcb(PCB *pcb);
void imprimir_registros(REGISTROS *registros);
void imprimir_lista_ids(t_list *tids);
void imprimir_hilos(t_list *threads);
void agregar_hilo(TCB *tcb, PCB *pcb);
void cambiar_estado(PCB *pcb, STATUS estado);
void cambiar_estado_hilo(TCB *tcb, STATUS estado);
void inicializar_registros(REGISTROS *registros); 
void imprimir_hilo(TCB* tcb);

#endif