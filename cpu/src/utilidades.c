#include "utilidades.h"

t_log *logger;
t_config *config;
char *ip_memoria;
char *puerto_memoria;
char *puerto_escucha_dispatch;
char *puerto_escucha_interrupt;
char *log_level;
CONTEXTO_CPU *contexto_en_ejecucion;
extern int fd_memoria;
extern int cliente_fd_dispatch;
sem_t sem_ejecucion;
pthread_mutex_t mutex_conexion_dispatch;
pthread_mutex_t mutex_conexion_memoria;

void iniciar_cpu(void)
{
    config = iniciar_config("cpu.config");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    log_level = config_get_string_value(config, "LOG_LEVEL");
    logger = iniciar_logger("logCpu.log", "Cpu", log_level_from_string(log_level));
    inicializar_semaforo(&sem_ejecucion,"sem_ejecucion",0);
    inicializar_mutex(&mutex_conexion_dispatch,"mutex_dispatch");
    inicializar_mutex(&mutex_conexion_memoria, "mutex memoria");
}

void inicializar_semaforo(sem_t* semaforo, char* nombre, int valor){
    if (sem_init(semaforo, 1, valor) != 0) {
        log_error(logger, "No se pudo inicializar el SEMAFORO: %s", nombre);
        exit(-1);
    }
}
void inicializar_mutex(pthread_mutex_t* mutex, char* nombre){
    if (pthread_mutex_init(mutex, NULL) != 0) {
        log_error(logger, "No se pudo inicializar el MUTEX: %s", nombre);
        exit(-1);
    }
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
    log_info(logger,"Solicitando contexto de: %d hilo: %d",pid,tid);
    contexto_en_ejecucion = solicitar_contexto_ejecucion(tid, pid); // esto es contexto_cpu pero la funcion devuelve contexto_hilo, ver
    printf("CTX DX En cpu dsps de pedir:%i\n", contexto_en_ejecucion->contexto_hilo->Registros->DX);
    //ejecutar();
    
    int recibido = EXEC_RECIBIDO;
    send(socket_cliente, &recibido,sizeof(op_code),0);
    sem_post(&sem_ejecucion);
    
    
    

    // sleep(1);
    // int resultado_ejecucion = OK_EJECUCION;
    // send(socket_cliente, &resultado_ejecucion,sizeof(op_code),0);
    // log_info(logger,"Ejecucion finalizada");
}

void procesar_fin_quantum(t_log *logger, int socket_cliente, op_code handshake){
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



CONTEXTO_CPU* solicitar_contexto_ejecucion(int tid, int pid){
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(SOLICITAR_CONTEXTO, buffer);
    enviar_paquete(paquete, fd_memoria);
    
    eliminar_paquete(paquete);
    int cod_op = recibir_operacion(fd_memoria);
    printf("cod op en cpu: %d\n",cod_op);
    switch(cod_op){ 
        case CONTEXTO_ENVIADO:
            CONTEXTO_CPU *contexto_cpu = recibir_contexto(fd_memoria);
            log_info(logger,"Contexto del proceso: %d, hilo %d recibido",contexto_cpu->contexto_proceso->pid,contexto_cpu->contexto_hilo->tid);
            return contexto_cpu;
        default:
            printf("default?\n");
            return 0;
    }
}



CONTEXTO_CPU *recibir_contexto(int socket_cliente){
    t_buffer* buffer = recibir_buffer_completo(socket_cliente);
    CONTEXTO_CPU *contexto_cpu = malloc(sizeof(CONTEXTO_CPU));
    contexto_cpu->contexto_hilo = extraer_contexto_hilo(buffer);
    contexto_cpu->contexto_proceso = extraer_contexto_proceso(buffer);
    return contexto_cpu;
}


uint32_t *obtenerRegistro(char *registro){
    if(string_equals_ignore_case(registro,"PC")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->PC;
    }else if(string_equals_ignore_case(registro,"AX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->AX;
    } else if(string_equals_ignore_case(registro,"BX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->BX;
    }else if(string_equals_ignore_case(registro,"CX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->CX;
    }else if(string_equals_ignore_case(registro,"DX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->DX;
    }else if(string_equals_ignore_case(registro,"EX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->EX;
    }else if(string_equals_ignore_case(registro,"FX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->FX;
    }else if(string_equals_ignore_case(registro,"GX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->GX;
    }else if(string_equals_ignore_case(registro,"HX")){
        return &contexto_en_ejecucion->contexto_hilo->Registros->HX;
    }else{
        log_error(logger, "No existe el registro indicado");
        exit(EXIT_FAILURE);
    }
}
