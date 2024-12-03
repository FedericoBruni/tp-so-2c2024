#include "planificador_corto_plazo.h"

extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_blocked;
extern char* algoritmo_planificacion;
extern t_queue *cola_ready;
extern t_queue *cola_blocked;
extern TCB *tcb_en_ejecucion;
extern t_log *logger;
extern int fd_cpu_interrupt;
extern int fd_cpu_dispatch;
extern sem_t sem_hay_ready;
extern sem_t sem_cpu_ejecutando;
extern t_list* colas_prioridades;
extern int quantum;
extern PCB *pcb_en_ejecucion;
extern sem_t sem_puede_ejecutar;



void planificador_corto_plazo(){
    
    if(string_equals_ignore_case(algoritmo_planificacion,"FIFO")){
        fifo();

    }else if(string_equals_ignore_case(algoritmo_planificacion, "PRIORIDADES")){
        prioridad();

    }else if(string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
        multinivel();
    } else {
        // no existe el algoritmo
        log_error(logger,"No se encontro el algoritmo de planificacion solicitado");
        exit(EXIT_FAILURE);
    }

    
}




void fifo(){
    while(1){ 
        sem_wait(&sem_hay_ready);
        sem_wait(&sem_puede_ejecutar);
        log_trace(logger, "Cola Ready:");
        imprimir_cola(cola_ready, mutex_ready);
        log_trace(logger, "Cola Blocked:");
        imprimir_cola(cola_blocked, mutex_blocked);
        TCB* tcb = desencolar(cola_ready,mutex_ready);
        if(tcb != NULL){
        tcb_en_ejecucion = tcb;
        log_trace(logger,"Planificando TCB %d,PID %d con algoritmo FIFO\n", tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid); 
        cambiar_estado_hilo(tcb, EXEC);
        pcb_en_ejecucion = tcb_en_ejecucion->pcb;
        enviar_exec_a_cpu(tcb->tid,tcb->pcb_pid);
        if (esperar_respuesta() == 1){
            continue;
        }
        }
    }
}

void prioridad(){
    while(1){
        sem_wait(&sem_hay_ready);
        list_sort(cola_ready->elements, (void*)comparar_prioridades);
        TCB* tcb =  desencolar(cola_ready, mutex_ready);
        if(tcb != NULL){
        tcb_en_ejecucion = tcb;
        log_info(logger,"Planificando TCB %d,PID %d con algoritmo PRIORIDADES\n", tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid); 
        cambiar_estado_hilo(tcb, EXEC);
        pcb_en_ejecucion = tcb_en_ejecucion->pcb;
        enviar_exec_a_cpu(tcb->tid,tcb->pcb_pid);
        esperar_respuesta();
        }
    }
}

void multinivel(){
        
    while(1){

        sem_wait(&sem_hay_ready);
        printear_colas_y_prioridades();
        COLA_PRIORIDAD* cola = obtener_cola_con_mayor_prioridad();
        log_info(logger, "cola elegida:%i",cola->prioridad);
        TCB *tcb = desencolar_multinivel(cola);
        if(tcb != NULL){
        tcb_en_ejecucion = tcb;
        log_info(logger,"Planificando TCB %d,PID %d con algoritmo MULTINIVEL\n", tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid);
        cambiar_estado_hilo(tcb, EXEC);
        pcb_en_ejecucion = tcb_en_ejecucion->pcb;
        pthread_t hilo_contador;
        pthread_create(&hilo_contador, NULL, (void *)fin_de_quantum, cola->quantum);
        pthread_detach(hilo_contador);
        enviar_exec_a_cpu(tcb->tid,tcb->pcb_pid);
        esperar_respuesta();
        pthread_cancel(hilo_contador);

        }
    }
}

void fin_de_quantum(int quantumCola){
    // habria q esperar a que cpu empiece a ejecutar?
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    sem_wait(&sem_cpu_ejecutando);
    sleep(quantumCola);
    int pid = tcb_en_ejecucion->pcb_pid;
    int tid = tcb_en_ejecucion->tid;
    enviar_fin_quantum(tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid);
    //TODO mandar interrupt a cpu
}


void printear_colas_y_prioridades(){
    for (unsigned int i = 0; i < list_size(colas_prioridades); i++){
        COLA_PRIORIDAD* cola_prioridad = list_get(colas_prioridades, i);
    }
}

int comparar_prioridades(TCB* tcb1, TCB* tcb2){
    return tcb1->prioridad <= tcb2->prioridad;
}

/*
int comparar_prioridades(void* a, void* b) {
    TCB* tcb1 = (TCB*)a;
    TCB* tcb2 = (TCB*)b;
    return tcb1->prioridad <= tcb2->prioridad;
}
*/