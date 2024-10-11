#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include "utils/utils.h"
#include <commons/collections/list.h>
#include "escuchar_mensajes_memoria.h"

void iniciar_memoria(void);
int conectarse_a_filesystem(void);
void terminar_ejecucion(int socket_conexion, int socket_servidor_kernel, int socket_servidor_cpu);
void enviar_contexto(int cliente_fd_cpu);
CONTEXTO_CPU* buscar_contextos(int tid, int pid);
#endif // UTILIDADES_H_