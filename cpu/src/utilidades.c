#include "utilidades.h"

t_log *logger;
t_config *config;
char *ip_memoria;
char *puerto_memoria;
char *puerto_escucha_dispatch;
char *puerto_escucha_interrupt;
char *log_level;

void iniciar_cpu(void)
{
    config = iniciar_config("cpu.config");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    log_level = config_get_string_value(config, "LOG_LEVEL");
    logger = iniciar_logger("logCpu.log", "Cpu", log_level_from_string(log_level));
}

int conectarse_a_memoria(void)
{
    return crear_conexion(ip_memoria, puerto_memoria, logger);
}

void terminar_ejecucion(int servidor_dispatch, int servidor_interrupt, int socket_memoria)
{
    log_info(logger, "Finalizando ejecución de CPU");
    close(servidor_dispatch);
    close(servidor_interrupt);
    close(socket_memoria);
    config_destroy(config);
    log_destroy(logger);
    exit(EXIT_SUCCESS);
}
