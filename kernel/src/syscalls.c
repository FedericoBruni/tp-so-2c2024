#include "syscalls.h"
extern int fd_memoria;
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
extern TCB *tcb_a_crear;
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
        log_warning(logger, "Este hilo de tid: %i  con prioridad ; %i no tiene los permisos para finalizar el proceso: %i",tcb->tid ,tcb->prioridad,tcb->pcb_pid);
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
    log_error(logger,"PID del HILO creado: %i",tcb->pcb->pid);
    encolar(cola_new_hilo,tcb,mutex_new);
    sem_post(&sem_crear_hilo);
    //señal d creacion
    // para mi se pone acá en ready y listo, por como dice el enunciado
    // Al momento de crear el nuevo hilo, deberá generar el nuevo TCB con un TID autoincremental y 
    // poner al mismo en el estado READY.
    //Dejo todo comentado pq no existe "pcb_en_ejecucion" y rompe
}


// Vamos a tener un pcb_en_ejecucion, y cada pcb tiene una ref al hilo que esta ejecutando en ese momento? o también 
// tener una variable tcb_en_ejecucion?
int THREAD_JOIN(int tid){
    int rta;
    rta = bloquear_hilo_syscall(tcb_en_ejecucion,tid);
    sem_post(&sem_syscall_fin);
    return rta;
}



void THREAD_EXIT(TCB *tcb) {
    // if (tcb->tid != 0) {
    //     log_warning(logger, "El hilo TID 0 debe invocar a THREAD_EXIT para finalizar el hilo.");
    //     return;
    // }

    log_info(logger, "Finalizando hilo con TID: %i del proceso con PID: %i", tcb->tid, tcb->pcb_pid);

    encolar(cola_finalizacion, tcb, mutex_exit);
    cambiar_estado_hilo(tcb, EXIT);
    //liberar_tcb(tcb);

    sem_post(&sem_finalizar_hilo);
}


// void THREAD_CANCEL(TCB *tcb) {
//     if (tcb->tid != 0) {
//         log_warning(logger, "El hilo TID 0 debe invocar a THREAD_CANCEL para cancelar el hilo.");
//         return;
//     }

//     log_info(logger, "Cancelando hilo con TID: %i del proceso con PID: %i", tcb->tid, tcb->pcb_pid);
//     encolar(cola_exit, tcb->pcb, mutex_exit);

//     cambiar_estado_hilo(tcb, EXIT);

//     liberar_tcb(tcb);

//     sem_post(&sem_finalizar_hilo);
// }

void THREAD_CANCEL(int tid, int pid) {
    TCB* tcb = buscar_tcb_en_cola(cola_ready,mutex_ready,tid,pid);
    if(tcb == NULL){
        tcb = buscar_tcb_en_cola(cola_blocked,mutex_blocked,tid,pid);
        if(tcb == NULL){
            if(tcb_en_ejecucion->tid == tid && tcb_en_ejecucion->pcb_pid==pid){
                tcb=tcb_en_ejecucion;
            }else{
            log_info(logger,"No se encontro el hilo <TID:%i>;<PID:%i>", tid, pid);
            return;
            }
        }   
    }
    
    log_info(logger, "Cancelando hilo con TID: %i del proceso con PID: %i", tcb->tid, tcb->pcb_pid);
    encolar(cola_finalizacion, tcb->pcb, mutex_exit);
    cambiar_estado_hilo(tcb, EXIT);
    liberar_tcb(tcb);
    sem_post(&sem_finalizar_hilo);
}

void MUTEX_CREATE(char* recurso){
    MUTEX* mutex = malloc(sizeof(MUTEX));
    mutex->recurso = recurso;
    mutex->binario = 1;
    mutex->asignadoA = NULL;
    PCB* pcb = pcb_en_ejecucion;
    //imprimir_pcb(pcb);
    mutex->cola_bloqueados = queue_create();
    list_add(pcb->mutex, mutex);
    list_add(mutex_sistema, mutex);

    log_error(logger,"MUTEX:%s CREADO",recurso);
    sem_post(&sem_syscall_fin);
    
}
/*
 se deberá verificar primero que exista el mutex solicitado y en caso de que exista y el mismo no se encuentre tomado 
 se deberá asignar dicho mutex al hilo correspondiente. En caso de que el mutex se encuentre tomado, el hilo que 
 realizó MUTEX_LOCK se bloqueará en la cola de bloqueados correspondiente a dicho mutex.
*/
void MUTEX_LOCK(char* recurso)
{
    MUTEX *mutex = existe_mutex(recurso);

    if(mutex == NULL){
        log_error(logger, "MUTEX LOCK NULL");
        return;
    }

    if(mutex->binario == 1){
        asignar_a_hilo_mutex(mutex, tcb_en_ejecucion);
        log_info(logger, "MUTEX LIBRE, PROCESO PUEDE USARLO");
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

    if(mutex == NULL){
        log_error(logger, "MUTEX UNLOCK NULL");
        return;
    }
    
    if(mutex->binario == 0 && mutex->asignadoA == tcb_en_ejecucion->tid) {
        log_warning(logger, "Desbloquear hilo mutex");
        desbloquear_hilo_mutex(mutex);
    }else{
        log_error(logger,"El hilo que desbloqueo el mutex no lo tiene");
    }

    sem_post(&sem_syscall_fin);
}

int DUMP_MEMORY(int pid, int tid){
    int rta = enviar_dump_memory(fd_memoria,tid,pid);
    sem_post(&sem_syscall_fin);
    return rta;
    // implementar logica
}









void IO(int tiempo){
    TCB *tcb_copia = malloc(sizeof(TCB));
    memcpy(tcb_copia,tcb_en_ejecucion,sizeof(TCB));
    IOStruct *io = malloc(sizeof(IOStruct));
    io->tiempo = tiempo;
    io->tcb=tcb_copia;
    encolar(cola_io,io,mutex_io);
    sem_post(&sem_io);
    sem_wait(&sem_io_iniciado);
    log_trace(logger,"IO Iniciado");
}





void ejecucion_io(){
    while (true) {
        sem_wait(&sem_io); 
        IOStruct *io = desencolar(cola_io,mutex_io);
        log_info(logger,"entro a ejecucion_io");
        log_trace(logger,"Tiempo: %d, TCB: %d",io->tiempo,io->tcb->tid);
        sem_post(&sem_io_iniciado);
        usleep(io->tiempo);
        log_info(logger, "## (%d:%d) finalizó IO y pasa a READY", io->tcb->pcb_pid, io->tcb->tid);
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
    }
}

void ejecucion_io2(int tiempo) {
    log_trace(logger, "A1");
    
    // bloquear el tcb en ejec
    encolar(cola_blocked, tcb_en_ejecucion, mutex_blocked);
    imprimir_cola(cola_ready, mutex_ready);
    imprimir_cola(cola_blocked, mutex_blocked);
    cambiar_estado_hilo(tcb_en_ejecucion, BLOCKED);
    sem_post(&sem_io);
    sleep(tiempo);
    log_info(logger, "## (%d:%d) finalizó IO y pasa a READY", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
    // pasar a ready el tcb en ejec
    TCB *tcb = desencolar(cola_blocked, mutex_blocked);
    encolar(cola_ready, tcb, mutex_ready);
    sem_post(&sem_hay_ready);
    log_trace(logger, "A2");
}
