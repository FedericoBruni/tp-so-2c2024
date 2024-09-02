#ifndef UTILS_H_
#define UTILS_H_
#include <commons/config.h>
#include <commons/log.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include <stdlib.h>

typedef enum
{
	HANDSHAKE_OK, 
	MENSAJE_A_MEMORIA,
	HANDSHAKE_KERNEL,
	HANDSHAKE_KERNEL_DISPATCH,
	HANDSHAKE_KERNEL_INTERRUPT,
	HANDSHAKE_ENTRADASALIDA,
	HANDSHAKE_MEMORIA,
	HANDSHAKE_CPU,
} op_code;

t_config* iniciar_config(char* ruta);
t_log* iniciar_logger(char *ruta_logger, char *nombre_logger, t_log_level level_logger);
int esperar_cliente(int socket_servidor, t_log *logger, char *cliente);
int crear_conexion(char *ip, char *puerto, t_log *logger);
int iniciar_servidor(t_log* logger, char* puerto);
int recibir_operacion(int socket_cliente);
void aceptar_handshake(t_log *logger, int socket_cliente, char*);
void rechazar_handshake(t_log *logger, int socket_cliente);
int realizar_handshake(t_log *logger, int socket_servidor, op_code handshake);
int enviar_handshake(t_log *logger, int socket_cliente, op_code handshake);




#endif  // UTILS_H_
