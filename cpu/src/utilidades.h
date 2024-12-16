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
#include <semaphore.h>
#include <ciclo_instruccion.h>

void iniciar_cpu(void);
int conectarse_a_memoria(void);
void terminar_ejecucion();
void liberar_contexto_proceso(CONTEXTO_CPU *contexto_proceso);
void recibir_exec(t_log *logger, int socket_cliente, op_code handshake);
void procesar_fin_quantum(t_log *logger, int socket_cliente, op_code handshake);
CONTEXTO_CPU* solicitar_contexto_ejecucion(int tid, int pid);
CONTEXTO_CPU *recibir_contexto(int socket_cliente);
uint32_t *obtenerRegistro(char *registro);
void inicializar_semaforo(sem_t* semaforo, char* nombre, int valor);
void inicializar_mutex(pthread_mutex_t* mutex, char* nombre);
#endif // UTILIDADES_H_