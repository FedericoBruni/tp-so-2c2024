#include "mensajeria.h"
extern int fd_cpu_dispatch;
extern int fd_cpu_interrupt;
extern char *desc_code_op[];
extern t_log *logger;
extern sem_t sem_cpu_ejecutando;
extern TCB* tcb_en_ejecucion;
extern PCB *pcb_en_ejecucion;
extern sem_t sem_puede_ejecutar;
extern sem_t sem_syscall_fin;
extern char* estado_lock;
extern t_queue *cola_ready;
extern t_queue *cola_blocked;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_blocked;
extern sem_t sem_finalizar_proceso;
extern sem_t sem_exec_recibido;
extern t_queue *cola_fin_pcb;
extern pthread_mutex_t mutex_exit;
extern char* algoritmo_planificacion;
extern t_list *colas_prioridades;

int solicitar_memoria(int socket_memoria, PCB *pcb, op_code cod_sol)
{

    t_buffer *buffer = crear_buffer();
    //cargar_pcb_al_buffer(buffer, pcb);
    cargar_int_al_buffer(buffer, pcb->pid);
    cargar_int_al_buffer(buffer, pcb->tamanio);
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
            log_trace(logger,"CPU ejecutando el hilo: %d del proceso: %d\n",tid,pid);
            sem_post(&sem_cpu_ejecutando);
            sem_post(&sem_exec_recibido);
            return 1;
        case SYSCALL_THREAD_CREATE:
            log_info(logger,"SYSCALL_THREAD_CREATE en enviar exec a cpu");
            break;    
        case SUSP_PROCESO: // este case estaba en esperar_respuesta();
            sem_post(&sem_puede_ejecutar);
        default:
            log_error(logger, "default");
            return 0;
    }
}

void enviar_fin_quantum(int tid, int pid){
    log_warning(logger,"TID:%d PID:%d FINALIZO QUANTUM",tid,pid);
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(FIN_QUANTUM, buffer);
    enviar_paquete(paquete, fd_cpu_interrupt);
    log_warning(logger,"ENIVE MENSAJE A CPU INTERRUPT TID:%d PD:%d ",tid,pid);
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

int esperar_respuesta(){
    log_warning(logger,"Esperando Respuesta");
    int resultado = 0;
    int operacion;
    while(1){
    log_error(logger,"ANTES DE RECIBIR LA OPERACION");
    operacion = recibir_operacion(fd_cpu_dispatch);
    log_error(logger,"OPERACION RECIBIDA");
    switch(operacion){ // se recibe con un Motivo por el q fue desalojado
        case DESALOJO_POR_QUANTUM:
            log_error(logger, "replanificando tid:%d pid:%d ",tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid);
            replanificar(tcb_en_ejecucion);
            log_info(logger,"## (%d:%d) - Desalojado por fin de Quantum", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            return 1;
        case OK_EJECUCION:
            log_info(logger,"## (%d:%d) - Solicito syscall: %s", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid, desc_code_op[operacion]);
            break;
        case SYSCALL_PROCESS_CREATE:
            //log_info(logger,"## (%d:%d) - Se crea el proceso - Estado: NEW", tcb_en_ejecucion->pcb_pid, 0);
            log_trace(logger,"PROCESS CREATE RECIBIDO");
            deserializar_process_create();
            sem_wait(&sem_syscall_fin);
            log_error(logger,"MANDO RTA");
            int proceso_creado = PROCESO_CREADO;
            send(fd_cpu_dispatch, &proceso_creado, sizeof(op_code), 0);
            break;
        case SYSCALL_THREAD_CREATE:
            deserializar_thread_create();
            sem_wait(&sem_syscall_fin);
            int hilo_creado = HILO_CREADO;
            send(fd_cpu_dispatch,&hilo_creado,sizeof(op_code),0);
            break;
        case SYSCALL_THREAD_JOIN:
            log_info(logger,"## (%d:%d) - Bloqueado por: <PTHREAD_JOIN>", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            int rta_join = deserializar_thread_join();
            sem_wait(&sem_syscall_fin);
            int hilo_join = HILO_JOINEADO;
            if(!rta_join)
                hilo_join = HILO_NO_JOINEADO;
            send(fd_cpu_dispatch,&hilo_join,sizeof(op_code),0);
            break;
        case SYSCALL_THREAD_CANCEL:
            log_info(logger, "## (%d:%d) Se cancela el hilo", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            deserializar_thread_cancel();
            sem_wait(&sem_syscall_fin);
            int hilo_cancel = HILO_CANCEL;
            send(fd_cpu_dispatch,&hilo_cancel,sizeof(op_code),0);
            break;
        case SYSCALL_MUTEX_CREATE:
            deserializar_mutex_create();
            sem_wait(&sem_syscall_fin);
            int mutex_creado = MUTEX_CREADO;
            send(fd_cpu_dispatch, &mutex_creado, sizeof(op_code), 0);
            break;
        case SYSCALL_MUTEX_LOCK:
            deserializar_mutex_lock();
            sem_wait(&sem_syscall_fin);
            int mutex_lock;
            if(string_equals_ignore_case(estado_lock, "LIBRE")){
                mutex_lock = MUTEX_LOCKEADO;
            }else{
                mutex_lock = LOCKEAR_HILO;
            }
            send(fd_cpu_dispatch, &mutex_lock, sizeof(op_code), 0);
            
            break;
        case SYSCALL_MUTEX_UNLOCK:
            log_info(logger,"## (%d:%d) - Solicito syscall: %s", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid, desc_code_op[operacion]);
            deserializar_mutex_unlock();
            sem_wait(&sem_syscall_fin);
            int mutex_unlock = MUTEX_UNLOCKEADO;
            send(fd_cpu_dispatch, &mutex_unlock, sizeof(op_code), 0);
            break;
        case SYSCALL_THREAD_EXIT:
            log_info(logger, "## (%d:%d) Finaliza el hilo", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            THREAD_EXIT(tcb_en_ejecucion);
            sem_wait(&sem_syscall_fin);
            int thread_exit = FIN_HILO;
            send(fd_cpu_dispatch, &thread_exit, sizeof(op_code), 0);
            break;
        case SYSCALL_PROCESS_EXIT:
            //log_info(logger, "## Finaliza el proceso %d", tcb_en_ejecucion->pcb_pid);
            PROCESS_EXIT(tcb_en_ejecucion);
            log_trace(logger,"ANTES DE SEM_SYS_FIN");
            sem_wait(&sem_syscall_fin);
            log_trace(logger,"DSPS DE SEM_SYS_FIN");
            int process_exit = FIN_PROCESO;
            send(fd_cpu_dispatch, &process_exit, sizeof(op_code), 0);
            log_trace(logger,"MANDO RTA PROC EXIT");
            break;
        case FIN_DE_ARCHIVO:
            //sleep(5);
            log_warning(logger, "Fin de archivo");
            if (debe_finalizar_proceso()){
                log_info(logger, "## Finaliza el proceso %d", tcb_en_ejecucion->pcb_pid);
                PROCESS_EXIT_ULTIMO_HILO(tcb_en_ejecucion); // x ahora, ver si queda o q
                //PROCESS_EXIT(list_get(tcb_en_ejecucion->pcb->threads, 0));
                
            }else{
                log_info(logger, "## (%d:%d) Finaliza el hilo", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
                THREAD_EXIT(tcb_en_ejecucion);
            }
            sem_post(&sem_puede_ejecutar);
            return 1;
        case SUSP_PROCESO:
            log_info(logger,"## (%d:%d) - Solicito syscall: %s", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid, desc_code_op[operacion]);
            sem_post(&sem_puede_ejecutar);
            return 1;
        case SYSCALL_DUMP_MEMORY:
            log_info(logger,"## (%d:%d) - Solicito syscall: %s", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid, desc_code_op[operacion]);
            int rta = deserializar_dump_memory();
            sem_wait(&sem_syscall_fin);
            int res_dump = MEM_DUMPEADA;
            if (!rta) {
                res_dump = FIN_PROCESO;
                PROCESS_EXIT(tcb_en_ejecucion);
                sem_wait(&sem_syscall_fin);
            }
            send(fd_cpu_dispatch, &res_dump, sizeof(op_code), 0);
            break;

        case SYSCALL_IO:
            log_info(logger,"## (%d:%d) - Bloqueado por: <PTHREAD_JOIN / MUTEX / IO>", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            deserializar_io();
            int io_solicitada = IO_SOLICITADA;
            send(fd_cpu_dispatch,&io_solicitada,sizeof(op_code),0);
            break;
        default:
            return 0;
        }

    }
}

bool debe_finalizar_proceso() {
    if (string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
        return 0; // x ahora para testear, ver la logica dsp
    } else {
        if (buscar_en_cola(cola_ready, mutex_ready, tcb_en_ejecucion->pcb_pid) || 
        buscar_en_cola(cola_blocked, mutex_blocked, tcb_en_ejecucion->pcb_pid)) return 0;
    }
    
    return 1;
    
}

void deserializar_process_create(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    
    char* archivo_pseudocodigo = extraer_string_del_buffer(buffer);
    int tam_archivo = extraer_int_del_buffer(buffer);
    int prio_hilo = extraer_int_del_buffer(buffer);
    SYS_PROCESS_CREATE(archivo_pseudocodigo,tam_archivo,prio_hilo);

}

void deserializar_thread_create(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char* archivo_pseudocodigo = extraer_string_del_buffer(buffer);
    int prio_hilo = extraer_int_del_buffer(buffer);
    THREAD_CREATE(pcb_en_ejecucion,archivo_pseudocodigo,prio_hilo);
}

int deserializar_thread_join(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int tid = extraer_int_del_buffer(buffer);
    return THREAD_JOIN(tid);
}

void deserializar_thread_cancel(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);
    
    THREAD_CANCEL(tid,pid);
}
void deserializar_mutex_create(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char *recurso = extraer_string_del_buffer(buffer);
    MUTEX_CREATE(recurso);
}
void deserializar_mutex_lock(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char *recurso = extraer_string_del_buffer(buffer);
    MUTEX_LOCK(recurso);
}
void deserializar_mutex_unlock(){
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    char *recurso = extraer_string_del_buffer(buffer);
    MUTEX_UNLOCK(recurso);
}
// void deserializar_thread_exit(){

//     //THREAD_EXIT(tid,pid);
// }
// void deserializar_process_exit(){
//     t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
//     int pid = extraer_int_del_buffer(buffer);
//     //PROCESS_EXIT(pid);
// }

int deserializar_dump_memory() {
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int pid = extraer_int_del_buffer(buffer);
    int tid = extraer_int_del_buffer(buffer);
    log_info(logger, "DUMP MEMORY de <PID:%i>,<TID:%i>", pid, tid);
    return DUMP_MEMORY(pid, tid);
}

void deserializar_io() {
    t_buffer* buffer = recibir_buffer_completo(fd_cpu_dispatch);
    int tiempo = extraer_int_del_buffer(buffer);
    IO(tiempo);
    
}

int enviar_dump_memory(int socket_memoria, int tid, int pid){
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(SOL_DUMP, buffer);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
    int cod_op = recibir_operacion(socket_memoria);
    if (cod_op == MEM_DUMPEADA)
    {
        return 1;
    }
    else if (cod_op == MEM_DUMP_ERROR)
    {
        log_trace(logger, "MEM DUMP ERROR");
        return 0;
    } else {
        return -1;
    }
}