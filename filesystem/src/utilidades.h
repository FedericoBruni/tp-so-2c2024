#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<commons/config.h>

extern t_log* logger;
extern t_config* config;

t_config* iniciar_config(void);
int iniciar_servidor(void);
void terminar_ejecucion(int socket);

#endif //UTILIDADES_H_
