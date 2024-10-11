#ifndef MENSAJERIA_H
#define MENSAJERIA_H
#include "utilidades.h"
#include "utils/utils.h"

int solicitar_memoria(int socket_memoria, PCB *pcb, op_code cod_sol);
int notificar_finalizacion_proceso(int socket_memoria, int pid, op_code operacion);
int solicitar_creacion_hilo(int socket_memoria, TCB *tcb ,op_code operacion);
int notificar_finalizacion_hilo(int socket_memoria, int tid, int pid,op_code operacion);
int enviar_exec_a_cpu(int tid, int pid);
void enviar_fin_quantum(int tid, int pid);
void esperar_respuesta();
#endif