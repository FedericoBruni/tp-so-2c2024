#include "mensajeria.h"
extern int fd_cpu_dispatch;
extern int fd_cpu_interrupt;
extern char *desc_code_op[];
extern t_log *logger;
extern sem_t sem_cpu_ejecutando;
extern TCB* tcb_en_ejecucion;
extern PCB *pcb_en_ejecucion;

int solicitar_memoria(int socket_memoria, PCB *pcb, op_code cod_sol)
{

    t_buffer *buffer = crear_buffer();
    cargar_pcb_al_buffer(buffer, pcb);
    t_paquete *paquete = crear_paquete(cod_sol, buffer);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    if (recibir_operacion(socket_memoria) == OK_SOLICITUD_MEMORIA_PROCESO)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int notificar_finalizacion_proceso(int socket_memoria, int pid, op_code operacion)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(operacion, buffer);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
    if (recibir_operacion(socket_memoria) == OK_FINAL_PROCESO)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int solicitar_creacion_hilo(int socket_memoria, TCB *tcb ,op_code operacion){
    t_buffer *buffer = crear_buffer();
    cargar_tcb_al_buffer(buffer,tcb);
    t_paquete *paquete = crear_paquete(operacion,buffer);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
    if (recibir_operacion(socket_memoria) == OK_CREACION_HILO) {
    
        return 1;
    }
    else
    {
        return 0;
    }
}

int notificar_finalizacion_hilo(int socket_memoria, int tid, int pid,op_code operacion)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(operacion, buffer);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
    if (recibir_operacion(socket_memoria) == OK_FINAL_HILO)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int enviar_exec_a_cpu(int tid, int pid){
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(ENVIAR_EXEC, buffer);
    enviar_paquete(paquete, fd_cpu_dispatch);
    
    eliminar_paquete(paquete);
    switch(recibir_operacion(fd_cpu_dispatch)){ // se recibe con un Motivo por el q fue desalojado
        case EXEC_RECIBIDO:
            log_info(logger,"CPU ejecutando el hilo: %d del proceso: %d\n",tid,pid);
            sem_post(&sem_cpu_ejecutando);
            return 1;
        default:
            return 0;
    }
}

void enviar_fin_quantum(int tid, int pid){
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(FIN_QUANTUM, buffer);
    enviar_paquete(paquete, fd_cpu_interrupt);
    eliminar_paquete(paquete);
    // habria q recibir un ok de que se recibió la interrupción?
    switch(recibir_operacion(fd_cpu_interrupt)){
        case OK_FIN_QUANTUM:
            log_info(logger,"finalizando por quantum");
            return 1;

        default:
            return 0;
    }
    
}

void esperar_respuesta(){
    switch(recibir_operacion(fd_cpu_dispatch)){ // se recibe con un Motivo por el q fue desalojado
        case DESALOJO_POR_QUANTUM:
            replanificar(tcb_en_ejecucion);
            log_info(logger,"Desalojo por quantum");
            break;
        case OK_EJECUCION:
            log_info(logger,"Ok ejecucion");
            break;
        case SYSCALL_PROCESS_CREATE:
            log_info(logger, "Syscall Process Create");
            deserializar_process_create();
            break;
        case SYSCALL_THREAD_CREATE:
            deserializar_thread_create();
            break;
        case SYSCALL_THREAD_JOIN:
            deserializar_thread_join();
            break;
        case SYSCALL_THREAD_CANCEL:
            deserializar_thread_cancel();
            break;
        case SYSCALL_MUTEX_CREATE:
            deserializar_mutex_create();
            break;
        case SYSCALL_MUTEX_LOCK:
            deserializar_mutex_lock();
            break;
        case SYSCALL_MUTEX_UNLOCK:
            deserializar_mutex_unlock();
            break;
        case SYSCALL_THREAD_EXIT:
            deserializar_thread_exit();;
            break;
        case SYSCALL_PROCESS_EXIT:
            deserializar_process_exit();
            break;
        default:
            return 0;
    }
}

void deserializar_process_create(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    
    char* archivo_pseudocodigo = extraer_string_del_buffer(buffer);
    int tam_archivo = extraer_int_del_buffer(buffer);
    int prio_hilo = extraer_int_del_buffer(buffer);
    PROCESS_CREATE(archivo_pseudocodigo,tam_archivo,prio_hilo);

}

void deserializar_thread_create(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char* archivo_pseudocodigo = extraer_string_del_buffer(buffer);
    int prio_hilo = extraer_int_del_buffer(buffer);
    THREAD_CREATE(pcb_en_ejecucion,archivo_pseudocodigo,prio_hilo);
}

void deserializar_thread_join(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int tid = extraer_int_del_buffer(buffer);
    THREAD_JOIN(tid);
}

void deserializar_thread_cancel(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int tid = extraer_int_del_buffer(fd_cpu_dispatch);
    int pid = extraer_int_del_buffer(fd_cpu_dispatch);
    //THREAD_CANCEL(tid,pid);
}
void deserializar_mutex_create(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char *recurso = extraer_string_del_buffer(fd_cpu_dispatch);
    MUTEX_CREATE(recurso);
}
void deserializar_mutex_lock(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char *recurso = extraer_string_del_buffer(fd_cpu_dispatch);
    MUTEX_LOCK(recurso);
}
void deserializar_mutex_unlock(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char *recurso = extraer_string_del_buffer(fd_cpu_dispatch);
    MUTEX_UNLOCK(recurso);
}
void deserializar_thread_exit(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int tid = extraer_int_del_buffer(fd_cpu_dispatch);
    int pid = extraer_int_del_buffer(fd_cpu_dispatch);
    //THREAD_EXIT(tid,pid);
}
void deserializar_process_exit(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int pid = extraer_int_del_buffer(fd_cpu_dispatch);
    //PROCESS_EXIT(pid);
}