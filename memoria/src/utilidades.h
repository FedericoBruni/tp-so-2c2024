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
#include <commons/collections/dictionary.h>
#include <commons/string.h>

void iniciar_memoria(void);
int conectarse_a_filesystem(void);
void terminar_ejecucion(int socket_conexion, int socket_servidor_kernel, int socket_servidor_cpu);
void enviar_contexto(int cliente_fd_dispatch);
CONTEXTO_CPU* buscar_contextos(int tid, int pid);
CONTEXTO_PROCESO *buscar_contexto_proceso(int pid);
void eliminar_hilo_y_contexto(int tid, int pid);
void actualizar_contexto(int cliente_fd_dispatch);
void cargar_archivo(char* nombre_archivo,int tid,int pid);
Particion *buscar_particion(int tamanio);
void finalizacion_de_proceso(int pid);
#endif // UTILIDADES_H_