#include "utilidades.h"

t_log* logger;
t_config* config;

t_config* iniciar_config(void){
    config = config_create("/home/utnso/tp-2024-2c-EnWindowsEraMasFacil/filesystem/filesystem.config");
    if(config == NULL){
        abort();
    }
    return config;
}

int iniciar_servidor(void){
    int socket_servidor;

    struct addrinfo hints, *server_info;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char* puerto = config_get_string_value(config,"PUERTO_ESCUCHA");


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

void terminar_ejecucion(int socket)
{
    close(socket);
    log_trace(logger,"Servidor cerrado");

}