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
extern int cliente_fd_interrupt;
sem_t sem_ejecucion;
sem_t sem_proceso_creado;
sem_t sem_mutex_creado;
sem_t sem_mutex_lockeado;
sem_t sem_mutex_unlockeado;
sem_t sem_thread_exit;
sem_t sem_process_exit;
sem_t sem_hilo_creado;
sem_t sem_join_hilo;
sem_t sem_hilo_cancel;
sem_t sem_io_solicitada;
sem_t sem_dump_mem;
sem_t sem_fin_q;
sem_t sem_interrupt_recibida;
sem_t sem_ctx_actualizado;
pthread_mutex_t mutex_conexion_dispatch;
pthread_mutex_t mutex_conexion_memoria;
pthread_mutex_t mutex_interrupt;
bool flag_interrupt = false;
bool fin_ciclo = false;


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
    inicializar_semaforo(&sem_proceso_creado, "sem_proceso_creado", 0);
    inicializar_semaforo(&sem_mutex_creado, "sem_mutex_creado", 0);
    inicializar_semaforo(&sem_mutex_lockeado, "sem_mutex_lockeado", 0);
    inicializar_semaforo(&sem_mutex_unlockeado, "sem_mutex_unlockeado", 0);
    inicializar_semaforo(&sem_thread_exit,"Finalizacion de hilo",0);
    inicializar_semaforo(&sem_process_exit,"Finalizacion de proceso",0);
    inicializar_semaforo(&sem_hilo_creado,"Hilo creado",0);
    inicializar_semaforo(&sem_join_hilo,"Hilo creado",0);
    inicializar_semaforo(&sem_hilo_cancel,"Hilo creado",0);
    inicializar_semaforo(&sem_io_solicitada, "sem_io_solicitada", 0);
    inicializar_semaforo(&sem_dump_mem, "sem dump mem", 0);
    inicializar_semaforo(&sem_fin_q,"fin q",0);
    inicializar_semaforo(&sem_interrupt_recibida, "interrupt recibida", 0);
    inicializar_semaforo(&sem_ctx_actualizado, "sem_ctx_actualizado", 0);
    inicializar_mutex(&mutex_conexion_dispatch,"mutex_dispatch");
    inicializar_mutex(&mutex_conexion_memoria, "mutex memoria");
    inicializar_mutex(&mutex_interrupt, "Mutex interrupt");


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

void liberar_contexto_proceso(CONTEXTO_CPU *contexto_proceso) {
    free(contexto_proceso->contexto_proceso);
    free(contexto_proceso->contexto_hilo->Registros);
    free(contexto_proceso->contexto_hilo->archivo_pseudocodigo);
    free(contexto_proceso->contexto_hilo);
    free(contexto_proceso);
}

void terminar_ejecucion()
{
    log_info(logger, "## Finalizando ejecución de CPU");
    close(cliente_fd_dispatch);
    close(cliente_fd_interrupt);
    close(fd_memoria);
    config_destroy(config);
    log_destroy(logger);
    liberar_contexto_proceso(contexto_en_ejecucion);
    
    

    exit(EXIT_SUCCESS);
}


void recibir_exec(t_log *logger, int socket_cliente, op_code handshake)
{

	t_buffer* buffer = recibir_buffer_completo(socket_cliente);
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);

    //solicitar_contexto_ejecucion(fd_memoria, tid, pid);
    if(contexto_en_ejecucion){
        liberar_contexto_proceso(contexto_en_ejecucion);
    }
    contexto_en_ejecucion = solicitar_contexto_ejecucion(tid, pid); // esto es contexto_cpu pero la funcion devuelve contexto_hilo, ver
    //ejecutar();
    
    int recibido = EXEC_RECIBIDO;
    send(socket_cliente, &recibido,sizeof(op_code),0);
    sem_post(&sem_ejecucion);
    free(buffer->stream);
    free(buffer);
    // sleep(1);
    // int resultado_ejecucion = OK_EJECUCION;
    // send(socket_cliente, &resultado_ejecucion,sizeof(op_code),0);
}

void procesar_fin_quantum(t_log *logger, int socket_cliente, op_code handshake){
    t_buffer* buffer = recibir_buffer_completo(socket_cliente);
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);
    if(contexto_en_ejecucion->contexto_hilo->tid == tid && contexto_en_ejecucion->contexto_hilo->pid == pid){
        pthread_mutex_lock(&mutex_interrupt);
        flag_interrupt = true;
        pthread_mutex_unlock(&mutex_interrupt);
    }
    sem_wait(&sem_fin_q);
    int resultado_interrupt = OK_FIN_QUANTUM;
    send(socket_cliente, &resultado_interrupt,sizeof(op_code),0);
    free(buffer->stream);
    free(buffer);
    // Falta actualizar ctx ejecucion, y ver si la interrupcion es del tid pid en ejecucion, si no, descartarla
}



CONTEXTO_CPU* solicitar_contexto_ejecucion(int tid, int pid){
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(SOLICITAR_CONTEXTO, buffer);
    log_info(logger,"## TID: <%d> - Solicito Contexto Ejecución",tid);
    enviar_paquete(paquete, fd_memoria);
    
    eliminar_paquete(paquete);
    int cod_op = recibir_operacion(fd_memoria);
    //printf("cod op en cpu: %d\n",cod_op);
    switch(cod_op){ 
        case CONTEXTO_ENVIADO:
            CONTEXTO_CPU *contexto_cpu = recibir_contexto(fd_memoria);
            return contexto_cpu;
        default:
            log_error(logger, "Error solicitando el contexto de ejecucion");
            return 0;
    }
}



CONTEXTO_CPU *recibir_contexto(int socket_cliente){
    t_buffer* buffer = recibir_buffer_completo(socket_cliente);
    CONTEXTO_CPU *contexto_cpu = malloc(sizeof(CONTEXTO_CPU));
    contexto_cpu->contexto_hilo = extraer_contexto_hilo(buffer);
    contexto_cpu->contexto_proceso = extraer_contexto_proceso(buffer);
    free(buffer->stream);
    free(buffer);
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
