#ifndef MENSAJERIA_H
#define MENSAJERIA_H
#include "utilidades.h"
#include "utils/utils.h"

int solicitar_memoria(int socket_memoria, int tamanio, op_code cod_sol);
int notificar_finalizacion_proceso(int socket_memoria, int pid, op_code operacion);
int solicitar_creacion_hilo(int socket_memoria, int tid, op_code cod_sol);
int notificar_finalizacion_hilo(int socket_memoria, int pid, op_code operacion);
#endif