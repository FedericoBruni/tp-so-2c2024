#include "utils.h"

char *desc_code_op[] = {
	"HANDSHAKE KERNEL - MEMORIA", "HANDSHAKE KERNEL - CPU DISPATCH", "HANDSHAKE KERNEL - CPU INTERRUPT",
	"HANDSHAKE CPU - MEMORIA", "HANDSHAKE MEMORIA - FILESYSTEM", "SOLICITAR MEMORIA PROCESO",
	"OK SOLICITUD MEMORIA PROCESO", "ERROR SOLICITUD MEMORIA PROCESO", "FINAL_PROCESO", "OK_FINAL_PROCESO", "ERROR_FINAL_PROCESO",
	"SOLICITAR_CREACION_HILO", "OK_CREACION_HILO", "ERROR_CREACION_HILO","FINAL_HILO","OK_FINAL_HILO", "CANCELAR_HILO","ENIVAR_EXEC", "EXEC_RECIBIDO","FIN_QUANTUM",
	"DESALOJO_POR_QUANTUM","OK_FIN_QUANTUM","OK_EJECUCION", "SOLICITAR_CONTEXTO", "CONTEXTO_ENVIADO", "ACTUALIZAR_CONTEXTO", "SYSCALL_PROCESS_CREATE", "SYSCALL_THREAD_CREATE", 
	"SYSCALL_THREAD_JOIN", "SYSCALL_THREAD_CANCEL", "SYSCALL_MUTEX_CREATE", "SYSCALL_MUTEX_LOCK", "SYSCALL_MUTEX_UNLOCK",
	"SYSCALL_THREAD_EXIT", "SYSCALL_PROCESS_EXIT"};

t_config *iniciar_config(char *ruta)
{
	t_config *config = config_create(ruta);
	if (config == NULL)
	{
		printf("Error al cargar el config: %s\n", ruta);
		exit(EXIT_FAILURE);
	}
	return config;
}

t_log *iniciar_logger(char *ruta_logger, char *nombre_logger, t_log_level level_logger)
{
	t_log *nuevo_logger = log_create(ruta_logger, nombre_logger, true, level_logger);
	if (nuevo_logger == NULL)
	{
		printf("Error al crear el logger %s.\n", ruta_logger);
		exit(EXIT_FAILURE);
	}
	return nuevo_logger;
}

int esperar_cliente(int socket_servidor, t_log *logger, char *cliente)
{

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	if (socket_cliente == -1)
	{
		log_error(logger, "Error al aceptar cliente: %s\n", cliente);
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Se conecto el cliente: %s\n", cliente);

	return socket_cliente;
}

int crear_conexion(char *ip, char *puerto, t_log *logger)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Crea el socket
	int socket_cliente = socket(server_info->ai_family,
								server_info->ai_socktype,
								server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	// continuando sobre cuando creamos elp socket del cliente

	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) != 0)
	{
		log_error(logger, "Error conectando al Servidor, apagado o invalido");
		socket_cliente = -1;
	}

	freeaddrinfo(server_info);
	if (socket_cliente != -1)
		log_info(logger, "Conexión exitosa");
	return socket_cliente;
}

int iniciar_servidor(t_log *logger, char *puerto)
{
	int socket_servidor;

	struct addrinfo hints, *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &server_info);

	// Creo el socket del server
	socket_servidor = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	if (socket_servidor == -1)
	{
		log_error(logger, "Error al crear servidor");
		close(socket_servidor);
		freeaddrinfo(server_info);
		abort();
	}
	// Asocio el socket a un puerto
	setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
	bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen);
	// Escucho Conexiones
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(server_info);
	log_trace(logger, "Servidor levantado y listo para escuchar en el puerto: %s", puerto);
	return socket_servidor;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;

	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void aceptar_handshake(t_log *logger, int socket_cliente, op_code handshake)
{
	int result_ok = 0;
	log_info(logger, "Recibido handshake: %s", desc_code_op[handshake]);
	send(socket_cliente, &result_ok, sizeof(int), 0);
}

void rechazar_handshake(t_log *logger, int socket_cliente)
{
	int result_error = -1;
	log_error(logger, "Recibido handshake de un modulo no autorizado, rechazando...");
	send(socket_cliente, &result_error, sizeof(int), 0);
}

int realizar_handshake(t_log *logger, int socket_servidor, op_code handshake)
{
	printf("a\n");
	if (enviar_handshake(logger, socket_servidor, handshake) == -1)
	{
		close(socket_servidor);
		log_error(logger, "No se pudo realizar el handshake con el servidor");
		printf("c\n");
		return -1;
	}
	printf("b\n");
	return 0;
}

int enviar_handshake(t_log *logger, int socket_cliente, op_code handshake)
{
	int resultado;
	send(socket_cliente, &handshake, sizeof(int), 0);
	recv(socket_cliente, &resultado, sizeof(int), MSG_WAITALL);
	if (socket_cliente == -1)
	{
		log_warning(logger, "Handshake enviado - Servidor apagado");
	}
	else if (resultado != -1)
	{
		log_info(logger, "Handshake OK %s", desc_code_op[handshake]);
	}
	else
	{
		log_error(logger, "Handshake rechazado");
	}

	return resultado;
}

///

void enviar_valor(int mensaje, int socket_cliente, op_code codigo)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = codigo;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

t_buffer *crear_buffer(void)
{
	t_buffer *buffer = malloc(sizeof(t_buffer));
	buffer->size = 0;
	buffer->stream = NULL;
	return buffer;
}

t_paquete *crear_paquete(op_code codigo_op, t_buffer *buffer)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	paquete->buffer = buffer;
	return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void *serializar_paquete(t_paquete *paquete, int bytes)
{
	void *magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2 * sizeof(int);
	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

//

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(t_log *logger, int socket_cliente)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list *recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void *buffer;
	t_list *valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while (desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		char *valor = malloc(tamanio);
		memcpy(valor, buffer + desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}


/*
t_buffer* buffer = recibir_buffer_completo(socket_cliente);
    TCB *tcb = malloc(sizeof(TCB));
    tcb->tid = extraer_int_del_buffer(buffer);
    tcb->prioridad = extraer_int_del_buffer(buffer);
    tcb->STATUS = extraer_int_del_buffer(buffer);
    tcb->Registros = extraer_registros_del_buffer(buffer);
    tcb->pcb_pid = extraer_int_del_buffer(buffer);
    tcb->pcb = NULL; // por ahora, dsp ver si lo necesitamos
    tcb->archivo_pseudocodigo = extraer_string_del_buffer(buffer);
    tcb->bloqueadoPor = extraer_int_del_buffer(buffer);
    return tcb;
*/
void cargar_tcb_al_buffer(t_buffer *buffer,TCB *tcb){
	cargar_int_al_buffer(buffer,tcb->tid);
	cargar_int_al_buffer(buffer,tcb->pcb_pid);
	cargar_string_al_buffer(buffer,tcb->archivo_pseudocodigo);
	cargar_registros_al_buffer(buffer,tcb->Registros);
}

void cargar_pcb_al_buffer(t_buffer* buffer, PCB* pcb){
	cargar_int_al_buffer(buffer, pcb->pid);
	cargar_int_al_buffer(buffer,pcb->tamanio);
	cargar_int_al_buffer(buffer, pcb->BASE);
	cargar_int_al_buffer(buffer, pcb->LIMITE);
}

void cargar_registros_al_buffer(t_buffer *buffer, REGISTROS* registros){
	cargar_int_al_buffer(buffer, registros->PC);
	cargar_int_al_buffer(buffer, registros->AX);
	cargar_int_al_buffer(buffer, registros->BX);
	cargar_int_al_buffer(buffer, registros->CX);
	cargar_int_al_buffer(buffer, registros->DX);
	cargar_int_al_buffer(buffer, registros->EX);
	cargar_int_al_buffer(buffer, registros->FX);
	cargar_int_al_buffer(buffer, registros->GX);
	cargar_int_al_buffer(buffer, registros->HX);
}

void cargar_int_al_buffer(t_buffer *buffer, int valor_int)
{
	cargar_datos_al_buffer(buffer, &valor_int, sizeof(int));
}

void cargar_string_al_buffer(t_buffer *buffer, char *valor_string)
{
	cargar_datos_al_buffer(buffer, valor_string, strlen(valor_string) + 1); // +1 por el /0
}

void cargar_datos_al_buffer(t_buffer *buffer, void *datos, int size_datos)
{
	if (buffer->size == 0)
	{
		buffer->stream = malloc(sizeof(int) + size_datos);
		memcpy(buffer->stream, &size_datos, sizeof(int));
		memcpy(buffer->stream + sizeof(int), datos, size_datos); // Desplaza '+ sizeof(int)' para que no pise lo que ya esta
	}
	else
	{
		buffer->stream = realloc(buffer->stream, buffer->size + sizeof(int) + size_datos);
		memcpy(buffer->stream + buffer->size, &size_datos, sizeof(int));
		memcpy(buffer->stream + buffer->size + sizeof(int), datos, size_datos);
	}
	buffer->size += sizeof(int);
	buffer->size += size_datos;
}

void *extraer_datos_del_buffer(t_buffer *buffer)
{
	if (buffer->size == 0)
	{
		printf("El buffer está vacio");
		exit(EXIT_FAILURE);
	}
	int size_datos;
	memcpy(&size_datos, buffer->stream, sizeof(int));
	void *datos = malloc(size_datos);
	memcpy(datos, buffer->stream + sizeof(int), size_datos);

	int nuevo_size = buffer->size - sizeof(int) - size_datos;

	if (nuevo_size == 0)
	{ // Se extrajeron todos los datos
		buffer->size = 0;
		free(buffer->stream);
		buffer->stream = NULL;
		return datos;
	}

	void *nuevo_stream = malloc(nuevo_size);
	memcpy(nuevo_stream, buffer->stream + sizeof(int) + size_datos, nuevo_size);
	free(buffer->stream);
	buffer->size = nuevo_size;
	buffer->stream = nuevo_stream;

	return datos;
}

int extraer_int_del_buffer(t_buffer *buffer)
{
	int *valor_int = extraer_datos_del_buffer(buffer);
	int valor_int_retornado = *valor_int;
	free(valor_int); // Libera la memoria que reservo extraer_datos_del_buffer
	return valor_int_retornado;
}

char *extraer_string_del_buffer(t_buffer *buffer)
{
	char *valor_string = extraer_datos_del_buffer(buffer); // Después de usar el string hay que liberar la memoria
	return valor_string;
}

REGISTROS *extraer_registros_del_buffer(t_buffer *buffer){
	REGISTROS *registros = malloc(sizeof(REGISTROS));
    registros->PC = extraer_int_del_buffer(buffer);
    registros->AX = extraer_int_del_buffer(buffer);
	registros->BX = extraer_int_del_buffer(buffer);
	registros->CX = extraer_int_del_buffer(buffer);
	registros->DX = extraer_int_del_buffer(buffer);
	registros->EX = extraer_int_del_buffer(buffer);
	registros->FX = extraer_int_del_buffer(buffer);
	registros->GX = extraer_int_del_buffer(buffer);
	registros->HX = extraer_int_del_buffer(buffer);
    return registros;
}

CONTEXTO_PROCESO *extraer_pcb_del_buffer(t_buffer *buffer){

    CONTEXTO_PROCESO *contexto = malloc(sizeof(CONTEXTO_PROCESO));
    contexto->pid = extraer_int_del_buffer(buffer);
	// contexto->BASE = extraer_int_del_buffer(buffer);
	// contexto->LIMITE = extraer_int_del_buffer(buffer);
	return contexto;
}


CONTEXTO_HILO *extraer_tcb_del_buffer(t_buffer *buffer){
    CONTEXTO_HILO *contexto = malloc(sizeof(CONTEXTO_HILO));
    contexto->tid = extraer_int_del_buffer(buffer);
    contexto->pid = extraer_int_del_buffer(buffer);
    contexto->archivo_pseudocodigo = extraer_string_del_buffer(buffer);
    contexto->Registros = extraer_registros_del_buffer(buffer);
    return contexto;
}



t_buffer *recibir_buffer_completo(int socket_cliente)
{ // Recibe todo lo enviado despues del OP_CODE ([cod_op][size][void*......])
	t_buffer *buffer = malloc(sizeof(t_buffer));

	if (recv(socket_cliente, &(buffer->size), sizeof(int), MSG_WAITALL) > 0)
	{ // Guardo el [size]
		buffer->stream = malloc(buffer->size);
		if (recv(socket_cliente, buffer->stream, buffer->size, MSG_WAITALL) > 0)
		{ // Guardo el [void*.......]
			return buffer;
		}
		else
		{
			printf("Error al recibir el buffer");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		printf("Error el recibir el tamaño del buffer");
		exit(EXIT_FAILURE);
	}
	return buffer;
}

void cargar_contexto_hilo(t_buffer* buffer, CONTEXTO_HILO *ctx){
	cargar_int_al_buffer(buffer,ctx->tid);
	cargar_int_al_buffer(buffer,ctx->pid);
	cargar_string_al_buffer(buffer,ctx->archivo_pseudocodigo);
	cargar_registros_al_buffer(buffer,ctx->Registros);
}

void cargar_contexto_proceso(t_buffer* buffer, CONTEXTO_PROCESO *ctx){
	cargar_int_al_buffer(buffer, ctx->pid);
	cargar_int_al_buffer(buffer, ctx->BASE);
	cargar_int_al_buffer(buffer, ctx->LIMITE);
}

CONTEXTO_HILO *extraer_contexto_hilo(t_buffer *buffer){
	CONTEXTO_HILO *ctx = malloc(sizeof(CONTEXTO_HILO));
	ctx->tid = extraer_int_del_buffer(buffer);
	ctx->pid = extraer_int_del_buffer(buffer);
	ctx->archivo_pseudocodigo = extraer_string_del_buffer(buffer);
	ctx->Registros = extraer_registros_del_buffer(buffer);
	return ctx;
}

CONTEXTO_PROCESO* extraer_contexto_proceso(t_buffer *buffer){
	CONTEXTO_PROCESO* ctx = malloc(sizeof(CONTEXTO_PROCESO));
	ctx->pid = extraer_int_del_buffer(buffer);
	ctx->BASE = extraer_int_del_buffer(buffer);
	ctx->LIMITE = extraer_int_del_buffer(buffer);
	return ctx;
}

