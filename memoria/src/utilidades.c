#include "utilidades.h"

t_log* logger;
t_config* config;
char* puerto_escucha;
char* ip_filesystem;
char* puerto_filesystem;
int* tam_memoria;
char* path_instrucciones;
int* retardo_respuesta;
char* esquema;
char* algoritmo_busqueda;
t_list* particiones;
char* log_level;



void iniciar_memoria(){
    config = iniciar_config("memoria.config");
    puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
    ip_filesystem = config_get_string_value(config,"IP_FILESYSTEM");
    puerto_filesystem = config_get_string_value(config,"PUERTO_FILESYSTEM");
    tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
    path_instrucciones = config_get_string_value(config,"PATH_INSTRUCCIONES");
    retardo_respuesta = config_get_int_value(config,"RETARDO_RESPUESTA");
    esquema = config_get_string_value(config,"ESQUEMA_MEMORIA");
    algoritmo_busqueda = config_get_string_value(config,"ALGORITMO_BUSQUEDA");
    particiones = config_get_array_value(config, "PARTICIONES");
    log_level = config_get_string_value(config,"LOG_LEVEL");

    logger = iniciar_logger("memoria.log","Memoria", log_level_from_string(log_level));
    
    
}

int conectarse_a_filesystem(void){
    return crear_conexion(ip_filesystem,puerto_filesystem,logger);
}

void terminar_ejecucion(int socket_conexion, int socket_servidor_kernel,int socket_servidor_cpu){
    close(socket_conexion);
    close(socket_servidor_kernel);
    close(socket_servidor_cpu);
    log_destroy(logger);
    config_destroy(config);
    exit(EXIT_SUCCESS);
}
