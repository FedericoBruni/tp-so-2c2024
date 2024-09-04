#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include "utils/utils.h"
#include <commons/collections/list.h>
#include "planificador_largo_plazo.h"

void iniciar_kernel(void);

int conectarse_a_cpu_interrupt(void);
int conectarse_a_cpu_dispatch(void);
int conectarse_a_memoria(void);
void terminar_ejecucion(int dispatch, int memoria, int interrupt);
int solicitar_memoria(int socket_memoria,int tamanio, op_code cod_sol);
void destruir_tcb(void* ptr_tcb);
void destruir_pcb(void* ptr_pcb);

#endif //UTILIDADES_H_