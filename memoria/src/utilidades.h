#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include "utils/utils.h"
#include <commons/collections/list.h>

void iniciar_memoria(void);
int conectarse_a_filesystem(void);
void terminar_ejecucion(int socket_conexion,int socket_servidor);
#endif //UTILIDADES_H_