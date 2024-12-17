#include "syscalls.h"
//extern int fd_memoria;
extern t_queue *cola_new;
extern t_queue *cola_new_hilo;
extern t_queue* cola_ready;
extern t_queue *cola_exit;
extern t_queue *cola_blocked;
extern t_queue *cola_finalizacion;
extern t_queue *cola_fin_pcb;
extern t_queue *cola_io;
extern t_log *logger;
extern t_list *procesos;
extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_exit;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_blocked;
extern sem_t sem_hay_ready;
extern PCB *pcb_en_ejecucion;
extern sem_t sem_crear_hilo;
extern sem_t sem_finalizar_proceso;
extern sem_t sem_finalizar_hilo;
extern sem_t sem_hay_new;
extern TCB *tcb_en_ejecucion;
extern t_list *mutex_sistema;
extern sem_t sem_syscall_fin;
char* estado_lock;
extern sem_t sem_io;
extern pthread_mutex_t mutex_io;
extern sem_t sem_io_iniciado;
extern char* algoritmo_planificacion;
extern bool fin_ciclo;
extern char *algoritmo_planificacion;

/*
PROCESS_CREATE, esta syscall recibirá 3 parámetros de la CPU, el primero será el nombre del archivo de pseudocódigo que
deberá ejecutar el proceso, el segundo parámetro es el tamaño del proceso en Memoria y el tercer parámetro es la prioridad
del hilo main (TID 0).
El Kernel c: 12
[INFO] 20:reará un nuevo PCB y un TCB asociado con TID 0 y lo dejará en estado NEW.
*/

void SYS_PROCESS_CREATE(char *archivo, int tamanio_memoria, int prioridad)
{
    PCB *pcb = crear_pcb(archivo, tamanio_memoria, prioridad);
    encolar(cola_new, pcb, mutex_new);
    sem_post(&sem_hay_new);
    
}

/*
PROCESS_EXIT, esta syscall finalizará el PCB correspondiente al TCB que ejecutó la instrucción,
enviando todos sus TCBs asociados a la cola de EXIT.
Esta instrucción sólo será llamada por el TID 0 del proceso y le deberá indicar a la memoria la finalización de dicho proceso.
*/

void PROCESS_EXIT(TCB *tcb)
{
    if (tcb->tid != 0)
    {
        //log_warning(logger, "Este hilo de tid: %i  con prioridad ; %i no tiene los permisos para finalizar el proceso: %i",tcb->tid ,tcb->prioridad,tcb->pcb_pid);
        return;
    }
    PCB *pcb = tcb->pcb;
    //mover_tcbs_exit(pcb);
    //queue_push(cola_finalizacion, pcb);
    encolar(cola_fin_pcb, pcb, mutex_exit); 
    sem_post(&sem_finalizar_proceso);

    //sem_post(&sem_);
    // Faltaría avisarle a Memoria??
    // Y sacar el pcb de la cola en la que está?
    // singal final_proceso
}

void PROCESS_EXIT_ULTIMO_HILO(TCB *tcb)
{

    PCB *pcb = tcb->pcb;
    mover_tcbs_exit(pcb);
    //queue_push(cola_finalizacion, pcb);
    encolar(cola_fin_pcb, pcb, mutex_exit); 
    sem_post(&sem_finalizar_proceso);

    //sem_post(&sem_);
    // Faltaría avisarle a Memoria??
    // Y sacar el pcb de la cola en la que está?
    // singal final_proceso
}

void THREAD_CREATE(PCB *pcb,char* archivo_pseudocodigo, int prioridad){
    
    TCB *tcb = crear_tcb(pcb, prioridad, archivo_pseudocodigo);
    encolar(cola_new_hilo,tcb,mutex_new);
    sem_post(&sem_crear_hilo);
}

int THREAD_JOIN(int tid){
    int rta;
    rta = bloquear_hilo_syscall(tcb_en_ejecucion,tid);
    sem_post(&sem_syscall_fin);
    return rta;
}



void THREAD_EXIT(TCB *tcb) {
    encolar(cola_finalizacion, tcb, mutex_exit);
    cambiar_estado_hilo(tcb, EXIT);
    sem_post(&sem_finalizar_hilo);
}

void THREAD_CANCEL(int tid, int pid) {
    TCB *tcb;
    if(string_equals_ignore_case(algoritmo_planificacion,"MULTINIVEL")){
        log_trace(logger,"multinivel");
        tcb = buscar_tcb_en_multinivel(tid,pid);
    }else{
        tcb = buscar_tcb_en_cola(cola_ready,mutex_ready,tid,pid);
    }
    if(tcb == NULL){
        tcb = buscar_tcb_en_cola(cola_blocked,mutex_blocked,tid,pid);
        if(tcb == NULL){
            if(tcb_en_ejecucion->tid == tid && tcb_en_ejecucion->pcb_pid==pid){
                tcb=tcb_en_ejecucion;
            }else{
            log_error(logger,"No se encontro el hilo (PID:TID) - (<%i>:<%i>)", pid, tid);
            sem_post(&sem_syscall_fin);
            return;
            }
        }   
    }

    encolar(cola_finalizacion, tcb, mutex_exit);
    cambiar_estado_hilo(tcb, EXIT);
    sem_post(&sem_finalizar_hilo);
}

void MUTEX_CREATE(char* recurso){
    MUTEX* mutex = malloc(sizeof(MUTEX));
    mutex->recurso = recurso;
    mutex->binario = 1;
    mutex->asignadoA = NULL;
    PCB* pcb = pcb_en_ejecucion;
    mutex->cola_bloqueados = queue_create();
    list_add(pcb->mutex, mutex);
    list_add(mutex_sistema, mutex);
    sem_post(&sem_syscall_fin);
    
}

void MUTEX_LOCK(char* recurso)
{
    MUTEX *mutex = existe_mutex(recurso);
    free(recurso);

    if(mutex == NULL){
        //log_error(logger, "MUTEX LOCK NULL");
        return;
    }

    if(mutex->binario == 1){
        asignar_a_hilo_mutex(mutex, tcb_en_ejecucion);
        estado_lock = "LIBRE";
    }else{
        bloquear_hilo_mutex(mutex->cola_bloqueados, tcb_en_ejecucion);
        estado_lock = "LOCKEADO";
        log_info(logger,"## (%d:%d) - Bloqueado por: <MUTEX>", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
    }
    sem_post(&sem_syscall_fin);
    
}

void MUTEX_UNLOCK(char *recurso){
    MUTEX *mutex = existe_mutex(recurso);
    free(recurso);

    if(mutex == NULL){
        //log_error(logger, "MUTEX UNLOCK NULL");
        
        return;
    }
    
    if(mutex->binario == 0 && mutex->asignadoA == tcb_en_ejecucion->tid) {
        desbloquear_hilo_mutex(mutex);
    }else{
        log_error(logger,"## El hilo que desbloqueo el mutex no lo tiene");
    }

    sem_post(&sem_syscall_fin);
}

int DUMP_MEMORY(int pid, int tid){
    int fd_memoria = conectarse_a_memoria();
    int rta = enviar_dump_memory(fd_memoria,tid,pid);
    close(fd_memoria);
    sem_post(&sem_syscall_fin);
    return rta;
    // implementar logica
}









void IO(int tiempo, TCB *tcb){
    IOStruct *io = malloc(sizeof(IOStruct));
    io->tiempo = tiempo;
    io->tcb=tcb;
    log_info(logger,"## (<%d>:<%d>) - Bloqueado por: <IO>",tcb->pcb_pid,tcb->tid);
    encolar(cola_io,io,mutex_io);
    sem_post(&sem_io);
    //sem_wait(&sem_io_iniciado);
}





void ejecucion_io(){
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (!fin_ciclo) {
        sem_wait(&sem_io); 
        if(fin_ciclo){
            return;
        }
        IOStruct *io = desencolar(cola_io,mutex_io);
        sem_post(&sem_io_iniciado);
        usleep(io->tiempo); // * 1000
        log_info(logger, "## (%d:%d) finalizó IO y pasa a READY", io->tcb->pcb_pid, io->tcb->tid);
        //printear_colas_y_prioridades();
        if(string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
            COLA_PRIORIDAD * cola = existe_cola_con_prioridad(io->tcb->prioridad);
            if(cola != NULL){
                encolar_multinivel(cola, io->tcb);
            } else{
                COLA_PRIORIDAD *cola_nueva = crear_multinivel(io->tcb);
                encolar_multinivel(cola_nueva, io->tcb);
            }         
        }else{
            encolar(cola_ready, io->tcb,mutex_ready);
        }
        sem_post(&sem_hay_ready);
        free(io);
    }
}
