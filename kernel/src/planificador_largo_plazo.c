#include "planificador_largo_plazo.h"
extern t_queue *cola_new;
extern t_queue *cola_ready;
extern t_queue *cola_finalizacion;
extern t_log *logger;
extern PCB *pcb_en_ejecucion;
extern int fd_memoria;
extern char *archivo_pseudocodigo;
extern int tamanio_proceso;
extern bool hilo_creacion_muerto;
extern bool hilo_fin_proc_muerto;
extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_exit;
extern pthread_mutex_t mutex_ready;
extern sem_t sem_hay_memoria;
extern TCB* tcb_a_crear;
extern sem_t sem_crear_hilo;
extern sem_t sem_finalizar_proceso;
extern sem_t sem_finalizar_hilo;
extern sem_t sem_hay_ready;
extern sem_t sem_hay_new;
extern sem_t memoria_libre;
extern char *algoritmo_planificacion;

void creacion_de_procesos(void)
{
    hilo_creacion_muerto = false;
    while (true)
    {
        sem_wait(&sem_hay_new);
        sem_wait(&memoria_libre);
        PCB* pcb = desencolar(cola_new,mutex_new);
        log_info(logger, "Solicitando memoria para el proceso: %d\n",pcb->pid);
        int resultado = solicitar_memoria(fd_memoria, pcb, SOLICITAR_MEMORIA_PROCESO);
        switch (resultado)
        {
        case 1:
            log_info(logger, "Memoria reservada correctamente");
            log_info(logger, "## (<PID>: %i) Se crea el proceso - Estado: NEW", pcb->pid);
            THREAD_CREATE(pcb,pcb->archivo_pseudocodigo, 0);
            
            

            break;

        case 0:
            log_warning(logger, "No hay espacio en memoria");
            encolar(cola_new, pcb,mutex_new);
            sem_wait(&sem_hay_memoria);
            sem_post(&sem_hay_new);
            break;
        }
        //break; // por ahora, q lo haga una sola vez pq no tenemos el semaforo
    hilo_creacion_muerto = true;
    }
}


void finalizacion_de_procesos(void)
{
    hilo_fin_proc_muerto = false;
    while (true)
    {
        sem_wait(&sem_finalizar_proceso);
        //pthread_mutex_lock(&mutex_exit);
        PCB *pcb = desencolar(cola_finalizacion,mutex_exit);
        log_info(logger, "intentando finalizar proceso con pid: %i", pcb->pid);
        int resultado = notificar_finalizacion_proceso(fd_memoria, pcb->pid, FINAL_PROCESO);
        switch (resultado)
        {
        case 1:
            log_info(logger, "## Finaliza el proceso <%i>", pcb->pid);
            liberar_pcb(pcb);
            sem_post(&sem_hay_memoria);
            break;
        case 0:
            log_error(logger, "Error al finalizar el proceso: %i\n", pcb->pid);
            queue_push(cola_finalizacion, pcb);
            break;
        }
        break;
    }
    hilo_fin_proc_muerto = true;
}

void creacion_de_hilos(void){
    
    while(true){
        
        sem_wait(&sem_crear_hilo);
        TCB* tcb = tcb_a_crear;
        //mandar por buffer a memoria
        int resultado = solicitar_creacion_hilo(fd_memoria, tcb, SOLICITAR_CREACION_HILO);
        switch (resultado){
            case 1:
                if(string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
                    COLA_PRIORIDAD * cola = existe_cola_con_prioridad(tcb->prioridad);
                    if(cola != NULL){
                        encolar_multinivel(cola, tcb);
                        log_info(logger,"## (<%i>:<%i>) Se crea el Hilo - Estado: READY", tcb->pcb_pid,tcb->tid);
                    } else{
                        COLA_PRIORIDAD *cola_nueva = crear_multinivel(tcb);
                        log_info(logger,"Se creo la cola multinivel de prioridad: %i",cola_nueva->prioridad);
                        encolar_multinivel(cola_nueva, tcb);
                        log_info(logger,"## (<%i>:<%i>) Se crea el Hilo - Estado: READY", tcb->pcb_pid,tcb->tid);
                    }         
                }else{
                    encolar(cola_ready, tcb,mutex_ready);
                    log_info(logger,"## (<%i>:<%i>) Se crea el Hilo - Estado: READY", tcb->pcb_pid,tcb->tid);
                }
                sem_post(&sem_hay_ready);
                sem_post(&memoria_libre);
                break;
            case 0:
                log_error(logger, "Error al crear el hilo: %i\n", tcb->tid);
                break; 
        }
    }
}


void finalizacion_de_hilos(void)
{
    while (true)
    {
        sem_wait(&sem_finalizar_hilo); 
        TCB *tcb = desencolar(cola_finalizacion, mutex_exit);
        int tidBloqueante = tcb->tid;

        int resultado = notificar_finalizacion_hilo(fd_memoria, tcb->tid, tcb->pcb_pid,FINAL_HILO);
        switch (resultado)
        {
        case 1:
            log_info(logger, "## Finaliza el hilo <%i> del proceso <%i>", tcb->tid, tcb->pcb_pid);
            liberar_tcb(tcb);
            desbloquear_bloqueados_por_hilo(tidBloqueante);
            break;
        case 0:
            log_error(logger, "Error al finalizar el hilo: %i del proceso: %i", tcb->tid, tcb->pcb_pid);
            encolar(cola_finalizacion, tcb, mutex_exit);
            break;
        }
    }
}
