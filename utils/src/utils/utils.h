#ifndef UTILS_H_
#define UTILS_H_

#include <commons/config.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/collections/list.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

typedef enum
{
	HANDSHAKE_KERNEL_MEMORIA,
	HANDSHAKE_KERNEL_CPU_DISPATCH,
	HANDSHAKE_KERNEL_CPU_INTERRUPT,
	HANDSHAKE_CPU_MEMORIA,
	HANDSHAKE_MEMORIA_FS,
	SOLICITAR_MEMORIA_PROCESO,
	OK_SOLICITUD_MEMORIA_PROCESO,
	ERROR_SOLICITUD_MEMORIA_PROCESO,
	FINAL_PROCESO,
	OK_FINAL_PROCESO,
	ERROR_FINAL_PROCESO,
	SOLICITAR_CREACION_HILO,
	OK_CREACION_HILO,
	ERROR_CREACION_HILO,
	FINAL_HILO
} op_code;

typedef enum
{
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT
} STATUS;

typedef struct
{
	uint32_t PC;
	uint32_t AX;
	uint32_t BX;
	uint32_t CX;
	uint32_t DX;
	uint32_t EX;
	uint32_t FX;
	uint32_t GX;
	uint32_t HX;
} REGISTROS;

typedef struct
{

	int pid;
	t_list *tids;
	t_list *mutex;
	STATUS status;
	char *archivo_pseudocodigo;
	int tamanio;
	REGISTROS *Registros;
	int autoincremental_tcb;
	t_list *threads;
	int prioridad_main;

} PCB;

typedef struct
{
	int tid;
	int prioridad;
	STATUS status;
	REGISTROS *Registros;
	int pcb_pid;
	PCB *pcb;
	char* archivo_pseudocodigo;
} TCB;

typedef struct
{
	int size;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

t_config *iniciar_config(char *ruta);
t_log *iniciar_logger(char *ruta_logger, char *nombre_logger, t_log_level level_logger);
int esperar_cliente(int socket_servidor, t_log *logger, char *cliente);
int crear_conexion(char *ip, char *puerto, t_log *logger);
int iniciar_servidor(t_log *logger, char *puerto);
int recibir_operacion(int socket_cliente);
void aceptar_handshake(t_log *logger, int socket_cliente, op_code handshake);
void rechazar_handshake(t_log *logger, int socket_cliente);
int realizar_handshake(t_log *logger, int socket_servidor, op_code handshake);
int enviar_handshake(t_log *logger, int socket_cliente, op_code handshake);
void *recibir_buffer(int *, int);
t_list *recibir_paquete(int);
void recibir_mensaje(t_log *logger, int);
void enviar_valor(int mensaje, int socket_cliente, op_code codigo);
t_paquete *crear_paquete(op_code codigo_op, t_buffer *buffer);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete *paquete, int socket_cliente);
void eliminar_paquete(t_paquete *paquete);
void *serializar_paquete(t_paquete *paquete, int bytes);
t_buffer *recibir_buffer_completo(int socket_cliente);
void cargar_int_al_buffer(t_buffer *buffer, int valor_int);
void cargar_string_al_buffer(t_buffer *buffer, char *valor_string);
int extraer_int_del_buffer(t_buffer *buffer);
char *extraer_string_del_buffer(t_buffer *buffer);
t_buffer *crear_buffer(void);
#endif // UTILS_H_
