#include "utils.h"


t_config* iniciar_config(char* ruta){
    t_config* config = config_create(ruta);
    if(config == NULL){
        printf("Error al cargar el config: %s\n", ruta);
        exit(EXIT_FAILURE);
    }
    return config;
}

t_log* iniciar_logger(char *ruta_logger, char *nombre_logger, t_log_level level_logger){
	t_log *nuevo_logger = log_create(ruta_logger, nombre_logger, true, level_logger);
	if (nuevo_logger == NULL){
		printf("Error al crear el logger %s.\n", ruta_logger);
		exit(EXIT_FAILURE);
	}
	return nuevo_logger;
}

int esperar_cliente(int socket_servidor, t_log *logger, char *cliente){

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
    if(socket_cliente == -1){
        log_error(logger,"Error al aceptar cliente: %s\n", cliente);
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
    if (socket_cliente != -1) log_info(logger, "Conexión exitosa");
	return socket_cliente;
}

int iniciar_servidor(t_log* logger, char* puerto){
    int socket_servidor;

    struct addrinfo hints, *server_info;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    


    getaddrinfo(NULL,puerto,&hints,&server_info);

    //Creo el socket del server
    socket_servidor = socket(server_info->ai_family,server_info->ai_socktype,server_info->ai_protocol);
    if(socket_servidor == -1){
        log_error(logger,"Error al crear servidor");
        close(socket_servidor);
        freeaddrinfo(server_info);
        abort();
    }
    //Asocio el socket a un puerto
    setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEPORT,&(int){1},sizeof(int));
    bind(socket_servidor,server_info->ai_addr,server_info->ai_addrlen);
    //Escucho Conexiones
    listen(socket_servidor,SOMAXCONN);

    freeaddrinfo(server_info);
    log_trace(logger,"Servidor levantado y listo para escuchar en el puerto: %s",puerto);
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

void aceptar_handshake(t_log *logger, int socket_cliente, char* origen)
{
	int result_ok = 0;
	log_info(logger, "Recibido handshake: %s", origen);
	send(socket_cliente, &result_ok, sizeof(int), 0);
}

void rechazar_handshake(t_log *logger, int socket_cliente)
{
	int result_error = -1;
	log_error(logger, "Recibido handshake de un modulo no autorizado, rechazando...");
	send(socket_cliente, &result_error, sizeof(int), 0);
}

int realizar_handshake(t_log *logger, int socket_servidor, op_code handshake){

	if(enviar_handshake(logger, socket_servidor, handshake) == -1){
		close(socket_servidor);
		log_error(logger, "No se pudo realizar el handshake con el servidor");
		return -1;
	}
	return 0;
}

int enviar_handshake(t_log *logger, int socket_cliente, op_code handshake)
{
	int resultado;
	send(socket_cliente, &handshake, sizeof(int), 0);
	recv(socket_cliente, &resultado, sizeof(int), MSG_WAITALL);
	if (resultado != -1)
	{
		log_info(logger, "Handshake OK");
	}
	else
	{
		log_error(logger, "Handshake rechazado");
	}

	return resultado;
}