#include "utilidades.h"

t_log* logger;
t_config* config;
char* ip_memoria;
char* puerto_memoria;
char*ip_cpu;
char* puerto_cpu_dispatch;
char* puerto_cpu_interrupt;
char* algoritmo_planificacion;
int* quantum;
char* log_level;

void iniciar_kernel(void){
    config = iniciar_config("kernel.config");
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
    ip_cpu = config_get_string_value(config,"IP_CPU");
    puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
    algoritmo_planificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
    quantum = config_get_int_value(config,"QUANTUM");
    log_level = config_get_string_value(config,"LOG_LEVEL");
    logger = iniciar_logger("logKernel.log","Kernel",log_level_from_string(log_level));
}

int conectarse_a_cpu_interrupt(void){
    return crear_conexion(ip_cpu, puerto_cpu_interrupt,logger);
}

int conectarse_a_cpu_dispatch(void){
    return crear_conexion(ip_cpu, puerto_cpu_dispatch,logger);
}

int conectarse_a_memoria(void){
    return crear_conexion(ip_memoria, puerto_memoria,logger);
}

void terminar_ejecucion(int dispatch,int memoria,int interrupt){
    log_info(logger,"Finalizando ejecucion");
    close(dispatch);
    close(memoria);
    close(interrupt);
    log_destroy(logger);
    config_destroy(config);
}