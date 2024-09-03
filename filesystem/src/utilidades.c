#include "utilidades.h"


t_log* logger;
t_config* config;
char* puerto;
char* mount_dir;
int block_size;
int block_count;
int retardo_acceso_bloque;

void iniciar_filesystem(void) {
    config = iniciar_config("filesystem.config");
    char* log_level = config_get_string_value(config,"LOG_LEVEL");
    puerto = config_get_string_value(config,"PUERTO_ESCUCHA");
    mount_dir = config_get_string_value(config,"MOUNT_DIR");
    block_size = config_get_int_value(config,"BLOCK_SIZE");
    block_count = config_get_int_value(config,"BLOCK_COUNT");
    retardo_acceso_bloque = config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
    logger = iniciar_logger("logFS.log","FileSystem",log_level_from_string(log_level));
    
}

void terminar_ejecucion(int socket)
{
    log_info(logger, "Finalizando ejecución de FILESYSTEM");
    close(socket);
    log_destroy(logger);
    config_destroy(config);
    exit(EXIT_SUCCESS);

}