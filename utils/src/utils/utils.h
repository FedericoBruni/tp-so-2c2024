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
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

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
	FINAL_HILO,
	CANCELAR_HILO,
	OK_FINAL_HILO,
	ENVIAR_EXEC,
	EXEC_RECIBIDO,
	FIN_QUANTUM,
	DESALOJO_POR_QUANTUM,
	OK_FIN_QUANTUM,
	OK_EJECUCION,
	SOLICITAR_CONTEXTO,
	CONTEXTO_ENVIADO,
	ACTUALIZAR_CONTEXTO,
	SYSCALL_PROCESS_CREATE,
	SYSCALL_THREAD_CREATE,
	SYSCALL_THREAD_JOIN,
	SYSCALL_THREAD_CANCEL,
	SYSCALL_MUTEX_CREATE,
	SYSCALL_MUTEX_LOCK,
	SYSCALL_MUTEX_UNLOCK,
	SYSCALL_THREAD_EXIT,
	SYSCALL_PROCESS_EXIT,
	SOLICITAR_INSTRUCCION,
	PROXIMA_INSTRUCCION,
	EOF_INSTRUCCION,
	CONTEXTO_ACTUALIZADO_OK,
	CONTEXTO_ACTUALIZADO_ERROR,
	FIN_DE_ARCHIVO,
	PROCESO_CREADO,
	SUSP_PROCESO,
	SYSCALL_DUMP_MEMORY,
	SYSCALL_IO,
	MUTEX_CREADO,
	MUTEX_LOCKEADO,
	MUTEX_UNLOCKEADO,
	LOCKEAR_HILO,
	FIN_HILO,
	FIN_PROCESO,
	WRITE_MEM,
	WRITE_MEM_RTA,
	READ_MEM,
	READ_MEM_RTA,
	HILO_CREADO,
	HILO_JOINEADO,
	HILO_CANCEL,
	IO_SOLICITADA,
	MEM_DUMPEADA,
	SOL_DUMP,
	MEM_DUMP_ERROR,
	OK,
	HILO_NO_JOINEADO,
	SEGMENTATION_FAULT

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
	int autoincremental_tcb;
	t_list *threads;
	int prioridad_main;
	uint32_t BASE;
	uint32_t LIMITE;

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
	int bloqueadoPor;
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

typedef struct
{
	char *recurso;
	int binario;
	int asignadoA;
	t_queue *cola_bloqueados;
} MUTEX;


typedef struct{
	int pid;
	uint32_t BASE;
	uint32_t LIMITE;
} CONTEXTO_PROCESO;

typedef struct{
	int tid;
	int pid;
	char *archivo_pseudocodigo;
	REGISTROS *Registros;
} CONTEXTO_HILO;

typedef struct{
	CONTEXTO_PROCESO *contexto_proceso;
	CONTEXTO_HILO *contexto_hilo;
} CONTEXTO_CPU;

typedef struct{
	int tid;
	int pid;
	t_dictionary* instrucciones;
}CONTEXTO_ARCHIVO;

typedef struct{
	void* memoria_usuario;
	t_list* particiones;
}MEMORIA_USUARIO;

typedef struct{
	int inicio;
	int tamanio;
	bool estaOcupado; //0 no 1 si
}Particion;

typedef struct{
	int tiempo;
	TCB *tcb;
}IOStruct;



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
void cargar_registros_al_buffer(t_buffer *buffer, REGISTROS* registros);
void cargar_pcb_al_buffer(t_buffer *buffer, PCB *pcb);
void cargar_tcb_al_buffer(t_buffer *buffer,TCB *tcb);
REGISTROS *extraer_registros_del_buffer(t_buffer *buffer);
CONTEXTO_HILO *extraer_tcb_del_buffer(t_buffer *buffer);
CONTEXTO_PROCESO *extraer_pcb_del_buffer(t_buffer *buffer);
void cargar_contexto_hilo(t_buffer* buffer, CONTEXTO_HILO *ctx);
void cargar_contexto_proceso(t_buffer* buffer, CONTEXTO_PROCESO *ctx);
CONTEXTO_HILO *extraer_contexto_hilo(t_buffer *buffer);
CONTEXTO_PROCESO* extraer_contexto_proceso(t_buffer *buffer);

#endif // UTILS_H_
