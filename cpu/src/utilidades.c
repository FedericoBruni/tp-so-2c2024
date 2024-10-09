#include "utilidades.h"

t_log *logger;
t_config *config;
char *ip_memoria;
char *puerto_memoria;
char *puerto_escucha_dispatch;
char *puerto_escucha_interrupt;
char *log_level;
extern int fd_memoria;
extern int cliente_fd_dispatch;

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

void recibir_exec(t_log *logger, int socket_cliente, op_code handshake)
{

	t_buffer* buffer = recibir_buffer_completo(socket_cliente);
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);
    log_info(logger, "Recibido EXEC (%i y %i):",tid ,pid);

    //solicitar_contexto_ejecucion(fd_memoria, tid, pid);
    //ejecutar();

    sleep(5);
    int resultado_ejecucion = OK_EJECUCION;
    send(socket_cliente, &resultado_ejecucion,sizeof(op_code),0);
}

procesar_fin_quantum(t_log *logger, int socket_cliente, op_code handshake){
    t_buffer* buffer = recibir_buffer_completo(socket_cliente);
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);
    log_info(logger, "Recibida interrupcion de Fin de Quantum (%i y %i):",tid ,pid);
    int resultado_interrupt = OK_FIN_QUANTUM;
    send(socket_cliente, &resultado_interrupt,sizeof(op_code),0);
    sleep(1);
    int resultado_dispatch = DESALOJO_POR_QUANTUM;
    send(cliente_fd_dispatch,&resultado_dispatch,sizeof(op_code),0);
}