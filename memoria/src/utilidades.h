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
void enviar_instruccion(int cliente_fd_dispatch);
char* obtener_instruccion(int key, int pid, int tid);
CONTEXTO_HILO *buscar_contexto_hilo(int pid,int tid);
CONTEXTO_CPU *buscar_contextos(int tid, int pid);
CONTEXTO_PROCESO *buscar_contexto_proceso(int pid);
CONTEXTO_ARCHIVO *buscar_archivo(int pid, int tid);
void actualizar_contexto(int cliente_fd_dispatch);
void cargar_archivo(char* nombre_archivo,int tid,int pid);
Particion *buscar_particion(int tamanio);
Particion *buscar_first_dinamicas(int tamanio);
Particion *buscar_best_dinamicas(int tamanio);
Particion *buscar_worst_dinamicas(int tamanio);
Particion *buscar_first_fijas(int tamanio);
Particion *buscar_best_fijas(int tamanio);
Particion *buscar_worst_fijas(int tamanio);
void finalizacion_de_proceso(int pid);
void eliminar_hilo_y_contexto(int tid, int pid);
int leer_memoria(int direccion);
void escribir_memoria(int direccion, int valor);
void deserializar_write_mem(int cliente_fd_dispatch);
void deserializar_read_mem(int cliente_fd_dispatch);
void enviar_lectura(int dato);
void dump_memory(int tid,  int pid);
char *parse_contexto_cpu(CONTEXTO_CPU *contexto);

#endif // UTILIDADES_H_