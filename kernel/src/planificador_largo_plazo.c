#include "planificador_largo_plazo.h"
extern t_queue *cola_new;
extern t_queue *cola_ready;
extern t_queue *cola_finalizacion;
extern t_queue *cola_new_hilo;
extern t_queue *cola_fin_pcb;
extern t_log *logger;
extern PCB *pcb_en_ejecucion;
extern char *archivo_pseudocodigo;
extern int tamanio_proceso;
extern bool hilo_creacion_muerto;
extern bool hilo_fin_proc_muerto;
extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_exit;
extern pthread_mutex_t mutex_ready;
extern sem_t sem_hay_memoria;
extern sem_t sem_crear_hilo;
extern sem_t sem_finalizar_proceso;
extern sem_t sem_finalizar_hilo;
extern sem_t sem_hay_ready;
extern sem_t sem_hay_new;
extern sem_t memoria_libre;
extern sem_t sem_syscall_fin;
extern sem_t sem_puede_ejecutar;
extern char *algoritmo_planificacion;
extern pthread_mutex_t mutex_fd_memoria;
bool hay_mem = true;
extern bool fin_ciclo;
extern t_list *pcbs_aceptados;
extern pthread_mutex_t mutex_liberar_tcb;

void creacion_de_procesos(void)
{
    hilo_creacion_muerto = false;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (true)
    {

      // log_error(logger,"ANTES DE HAY NEW");
	 sem_wait(&sem_hay_new);
        if(!hay_mem){
        //log_error(logger,"NO HAT MEM IF");
	sem_post(&sem_syscall_fin);   
	 continue;
        }
       // log_error(logger,"ANTES DE HAY MEM");
	sem_wait(&sem_hay_memoria);
        if(fin_ciclo) return;
        //log_warning(logger, "Cola new");
        //imprimir_cola_new(cola_new, mutex_new);
        
        PCB* pcb = desencolar(cola_new,mutex_new);
        int fd_memoria = conectarse_a_memoria();
        pthread_mutex_lock(&mutex_fd_memoria);
        int resultado = solicitar_memoria(fd_memoria, pcb, SOLICITAR_MEMORIA_PROCESO);
        pthread_mutex_unlock(&mutex_fd_memoria);
        close(fd_memoria);
        switch (resultado)
        {
        case 1:
            list_add(pcbs_aceptados, pcb);
            log_info(logger, "## (<PID>: %i) Se crea el proceso - Estado: NEW", pcb->pid);
            THREAD_CREATE(pcb,pcb->archivo_pseudocodigo, pcb->prioridad_main);
            sem_post(&sem_hay_memoria);
            break;

        case 0:
            encolar(cola_new, pcb,mutex_new);
            //log_warning(logger, "Encolé a new porque no había memoria para el proceso: %i, Cola new: ", pcb->pid);
            sem_post(&sem_syscall_fin);
            hay_mem = false;
            break;
        }
        //break; // por ahora, q lo haga una sola vez pq no tenemos el semaforo
    hilo_creacion_muerto = true;
    }
}


void finalizacion_de_procesos(void)
{
    hilo_fin_proc_muerto = false;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (true)
    {
        //log_error(logger,"ANTES DE FINALIZAR PROC");
        sem_wait(&sem_finalizar_proceso);
        if(fin_ciclo) return;
        //log_error(logger,"dsps DE FINALIZAR PROC");
        //pthread_mutex_lock(&mutex_exit);
        PCB *pcb = desencolar(cola_fin_pcb,mutex_exit);
        int fd_memoria = conectarse_a_memoria();
        pthread_mutex_lock(&mutex_fd_memoria);
        int resultado = notificar_finalizacion_proceso(fd_memoria, pcb->pid, FINAL_PROCESO);
        pthread_mutex_unlock(&mutex_fd_memoria);
        close(fd_memoria);
        switch (resultado)
        {
        case 1:
            list_remove_element(pcbs_aceptados, pcb);
            log_info(logger, "## Finaliza el proceso <%i>", pcb->pid);
            //printear_colas_y_prioridades();
            liberar_pcb(pcb);
            //printear_colas_y_prioridades();
            //imprimir_cola_new(cola_new,mutex_new);
            hay_mem=true;
	    if(queue_size(cola_new)>=1){
                sem_post(&sem_hay_new);
            }
            sem_post(&sem_syscall_fin);
            sem_post(&sem_hay_memoria);
            //hay_mem=true;
            break;
        case 0:
            log_error(logger, "## Error al finalizar el proceso: <%i>", pcb->pid);
            queue_push(cola_finalizacion, pcb);
            break;
        }
    }
    hilo_fin_proc_muerto = true;
}

void creacion_de_hilos(void){
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(true){
        
        sem_wait(&sem_crear_hilo);
        if(fin_ciclo) return;
        TCB* tcb = desencolar(cola_new_hilo,mutex_new);
        //mandar por buffer a memoria
        int fd_memoria = conectarse_a_memoria();
        pthread_mutex_lock(&mutex_fd_memoria);
        int resultado = solicitar_creacion_hilo(fd_memoria, tcb, SOLICITAR_CREACION_HILO);
        pthread_mutex_unlock(&mutex_fd_memoria);
        close(fd_memoria);
        switch (resultado){
            case 1:
                if(string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
                    COLA_PRIORIDAD * cola = existe_cola_con_prioridad(tcb->prioridad);
                    if(cola != NULL){
                        encolar_multinivel(cola, tcb);
                    } else{
                        COLA_PRIORIDAD *cola_nueva = crear_multinivel(tcb);
                        log_trace(logger,"## Creando Cola Multinivel - Prioridad: <%d>",cola_nueva->prioridad);
                        encolar_multinivel(cola_nueva, tcb);
                    }         
                }else{
                    encolar(cola_ready, tcb,mutex_ready);
                    
                }
                log_info(logger,"## (<%i>:<%i>) Se crea el Hilo - Estado: READY", tcb->pcb_pid,tcb->tid);
                sem_post(&sem_hay_ready);
                sem_post(&sem_syscall_fin);
                break;
            case 0:
                log_error(logger, "## Error al crear el hilo: <%i>", tcb->tid);
                break; 
        }
    }
}

void finalizacion_de_hilos(void)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (true)
    {
        sem_wait(&sem_finalizar_hilo); 
        if(fin_ciclo) return;
        TCB *tcb = desencolar(cola_finalizacion, mutex_exit);
        int tidBloqueante = tcb->tid;
        int pidhilo = tcb->pcb_pid;
        int fd_memoria = conectarse_a_memoria();
        pthread_mutex_lock(&mutex_fd_memoria);
        int resultado = notificar_finalizacion_hilo(fd_memoria, tcb->tid, tcb->pcb_pid,FINAL_HILO);
        pthread_mutex_unlock(&mutex_fd_memoria);
        close(fd_memoria);
        switch (resultado)
        {
        case 1:
            log_info(logger, "## (<%d>:<%d>) Finaliza el hilo", tcb->pcb_pid,tcb->tid);
            list_remove_element(tcb->pcb->tids, tcb->tid);
            list_remove_element(tcb->pcb->threads, tcb);
            pthread_mutex_lock(&mutex_liberar_tcb);
            liberar_tcb(tcb);
            pthread_mutex_unlock(&mutex_liberar_tcb);
            desbloquear_bloqueados_por_hilo(tidBloqueante,pidhilo);
            sem_post(&sem_syscall_fin);
            break;
        case 0:
            log_error(logger, "## (PID:TID) - (<%d>:<%d>) - Error al finalizar el hilo", tcb->pcb_pid, tcb->tid);
            encolar(cola_finalizacion, tcb, mutex_exit);
            break;
        }
    }
}
