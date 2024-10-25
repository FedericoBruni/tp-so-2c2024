#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include "utils/utils.h"
#include <commons/collections/list.h>
#include <pthread.h>
#include "escuchar_kernel.h"
#include "mensajeria.h"

void iniciar_cpu(void);
int conectarse_a_memoria(void);
void terminar_ejecucion_a_memoria(void);
void recibir_exec(t_log *logger, int socket_cliente, op_code handshake);
void procesar_fin_quantum(t_log *logger, int socket_cliente, op_code handshake);
CONTEXTO_HILO* solicitar_contexto_ejecucion(int tid, int pid);
CONTEXTO_CPU *recibir_contexto(int socket_cliente);
uint32_t *obtenerRegistro(char *registro);
#endif // UTILIDADES_H_