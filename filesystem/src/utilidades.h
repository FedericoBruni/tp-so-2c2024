#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include <assert.h>
#include <commons/config.h>
#include "utils/utils.h"
#include <pthread.h>
#include "escuchar_memoria.h"

// extern t_log* logger;
// extern t_config* config;

void iniciar_filesystem(void);
void crear_bitmap();
void crear_bloques_de_datos();
bool verificar_espacio_disponible(int tam_archivo);
int cant_bloques_libres();
void crear_archivo_metadata(int pid, int tid, int tamanio);
void terminar_ejecucion(int socket);

#endif // UTILIDADES_H_
