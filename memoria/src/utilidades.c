#include "utilidades.h"

t_log *logger;
t_config *config;
char *puerto_escucha;
char *ip_filesystem;
char *puerto_filesystem;
int *tam_memoria;
char *path_instrucciones;
int *retardo_respuesta;
char *esquema;
char *algoritmo_busqueda;
t_list *particiones;
char *log_level;
t_list *contextos_procesos;
t_list *contextos_hilos;

void iniciar_memoria()
{
    config = iniciar_config("memoria.config");
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
    puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
    esquema = config_get_string_value(config, "ESQUEMA_MEMORIA");
    algoritmo_busqueda = config_get_string_value(config, "ALGORITMO_BUSQUEDA");
    particiones = config_get_array_value(config, "PARTICIONES");
    log_level = config_get_string_value(config, "LOG_LEVEL");
    logger = iniciar_logger("memoria.log", "Memoria", log_level_from_string(log_level));
    contextos_procesos = list_create();
    contextos_hilos = list_create();
}

int conectarse_a_filesystem(void)
{
    return crear_conexion(ip_filesystem, puerto_filesystem, logger);
}

void terminar_ejecucion(int socket_conexion, int socket_servidor_kernel, int socket_servidor_cpu)
{
    log_info(logger, "Finalizando ejecución de MEMORIA");
    close(socket_conexion);
    close(socket_servidor_kernel);
    close(socket_servidor_cpu);
    log_destroy(logger);
    config_destroy(config);
    exit(EXIT_SUCCESS);
}

void enviar_contexto(int cliente_fd_dispatch){
    t_buffer *buffer = recibir_buffer_completo(cliente_fd_dispatch);
    
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);

    CONTEXTO_CPU *contexto_cpu = malloc(sizeof(CONTEXTO_CPU));
    contexto_cpu = buscar_contextos(tid,pid);
    
    t_buffer *bufferRta = crear_buffer();
    cargar_contexto_hilo(bufferRta, contexto_cpu->contexto_hilo);
    cargar_contexto_proceso(bufferRta, contexto_cpu->contexto_proceso);
    t_paquete *paquete = crear_paquete(CONTEXTO_ENVIADO,bufferRta);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
    printf("Contexto enviado -> TID: %i, PID: %i\n", tid, pid);
    //enviar_contexto(cliente_fd_dispatch,contexto_cpu);
    //int rta_sol_mem = CONTEXTO_ENVIADO;
    //send(cliente_fd_dispatch, &rta_sol_mem, sizeof(op_code), 0);
    
}

CONTEXTO_CPU* buscar_contextos(int tid, int pid){
    bool _hayHilo(void *ptr){
        CONTEXTO_HILO *ctx_hilo = (CONTEXTO_HILO*) ptr;
        return ctx_hilo->tid == tid && ctx_hilo->pid == pid;
    }

    bool _hayProceso(void *ptr2){
        CONTEXTO_PROCESO *ctx_proc = (CONTEXTO_PROCESO*) ptr2;
        return ctx_proc->pid == pid;
    }

    CONTEXTO_HILO *contexto_hilo = list_find(contextos_hilos,_hayHilo);
    CONTEXTO_PROCESO *contexto_proceso = list_find(contextos_procesos, _hayProceso);

    CONTEXTO_CPU *contexto_cpu = malloc(sizeof(CONTEXTO_CPU));
    contexto_cpu->contexto_hilo = contexto_hilo;
    contexto_cpu->contexto_proceso = contexto_proceso;
    return contexto_cpu;
}


// void enviar_contexto(int cliente_fd_dispatch, CONTEXTO_CPU *contexto_cpu){
//     t_buffer *buffer = crear_buffer();
//     cargar_contexto_hilo(buffer, contexto_cpu->contexto_hilo);
//     cargar_contexto_proceso(buffer, contexto_cpu->contexto_proceso);

//     t_paquete *paquete = crear_paquete(CONTEXTO_ENVIADO,buffer);
//     enviar_paquete(paquete, cliente_fd_dispatch);
//     eliminar_paquete(paquete);

    

// }
void eliminar_hilo_y_contexto(int tid, int pid) {
    // bool _es_hilo(void *ptr) {
    //     CONTEXTO_HILO *ctx_hilo = (CONTEXTO_HILO *)ptr;
    //     return ctx_hilo->tid == tid && ctx_hilo->pid == pid;
    // }

    // bool _es_proceso(void *ptr) {
    //     CONTEXTO_PROCESO *ctx_proceso = (CONTEXTO_PROCESO *)ptr;
    //     return ctx_proceso->pid == pid;
    // }

    // CONTEXTO_HILO *contexto_hilo = list_find(contextos_hilos, _es_hilo);
    // if (contexto_hilo != NULL) {
    //     list_remove_and_destroy_by_condition(contextos_hilos, _es_hilo, free);
    //     log_info(logger, "Contexto del hilo con TID %d eliminado correctamente.", tid);

    //     t_list *hilos_restantes = list_filter(contextos_hilos, _es_hilo);
    //     if (list_is_empty(hilos_restantes)) {
    //         CONTEXTO_PROCESO *contexto_proceso = list_find(contextos_procesos, _es_proceso);
    //         if (contexto_proceso != NULL) {
    //             list_remove_and_destroy_by_condition(contexto_procesos, _es_proceso, free);
    //             log_info(logger, "Contexto del proceso con PID %d eliminado ya que no tiene hilos activos.", pid);
    //         }
    //     }
    //     list_destroy(hilos_restantes);
    // } else {
    //     log_error(logger, "No se encontro un contexto de hilo con TID %d.", tid);
    // }
}

//int list_index_of(t_list* contextos, CONTEXTO_PROCESO *contexto_proceso);

void actualizar_contexto(int cliente_fd_dispatch){
    
    t_buffer *buffer = recibir_buffer_completo(cliente_fd_dispatch);
    CONTEXTO_HILO *ctx_hilo = malloc(sizeof(CONTEXTO_HILO));
    CONTEXTO_PROCESO *ctx_proceso = malloc(sizeof(CONTEXTO_PROCESO));
    ctx_hilo = extraer_contexto_hilo(buffer);
    ctx_proceso = extraer_contexto_proceso(buffer);
    
    bool _es_hilo(void *ptr) {
        CONTEXTO_HILO *ctx_hilo_en_memoria = (CONTEXTO_HILO *)ptr;
        return ctx_hilo_en_memoria->tid == ctx_hilo->tid && ctx_hilo_en_memoria->pid == ctx_hilo->pid;
    }

    bool _es_proceso(void *ptr) {
        CONTEXTO_PROCESO *ctx_proceso_en_memoria = (CONTEXTO_PROCESO *)ptr;
        return ctx_proceso_en_memoria->pid == ctx_proceso->pid;
    }

    CONTEXTO_HILO *hilo_actual = list_find(contextos_hilos, _es_hilo);
    CONTEXTO_PROCESO *proceso_actual = list_find(contextos_procesos, _es_proceso);

    if (hilo_actual && proceso_actual) {
        memcpy(hilo_actual->Registros, ctx_hilo->Registros, sizeof(REGISTROS));
        log_info(logger, "Contexto de hilo y proceso actualizado para TID: %d y PID: %d", ctx_hilo->tid, ctx_hilo->pid);
    } else {
        log_error(logger, "No se encontro el contexto para TID: %d y PID: %d", ctx_hilo->tid, ctx_hilo->pid);
    }
}




