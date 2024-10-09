#include "planificador_corto_plazo.h"

extern pthread_mutex_t mutex_ready;
extern char* algoritmo_planificacion;
extern t_queue *cola_ready;
extern TCB *tcb_en_ejecucion;
extern t_log *logger;
extern int fd_cpu_interrupt;
extern int fd_cpu_dispatch;
extern sem_t sem_hay_ready;
extern t_list* colas_prioridades;
extern int quantum;



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
        TCB* tcb = desencolar(cola_ready,mutex_ready);
        tcb_en_ejecucion = tcb;
        log_info(logger,"Planificando TCB %d,PID %d con algoritmo FIFO\n", tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid); 
        cambiar_estado_hilo(tcb, EXEC);
        enviar_exec_a_cpu(tcb->tid,tcb->pcb_pid);

    }
}

void prioridad(){
    while(1){
        sem_wait(&sem_hay_ready);     
        list_sort(cola_ready->elements, (void*)comparar_prioridades);
        TCB* tcb =  desencolar(cola_ready, mutex_ready);
        tcb_en_ejecucion = tcb;
        log_info(logger,"Planificando TCB %d,PID %d con algoritmo PRIORIDADES\n", tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid); 
        cambiar_estado_hilo(tcb, EXEC);
        enviar_exec_a_cpu(tcb->tid,tcb->pcb_pid);
    }
}

void multinivel(){
        pthread_t hilo_contador;
        pthread_create(&hilo_contador, NULL, (void *)fin_de_quantum, NULL);
        pthread_detach(hilo_contador);
    while(1){
        sem_wait(&sem_hay_ready);
        printear_colas_y_prioridades();
        COLA_PRIORIDAD* cola = obtener_cola_con_mayor_prioridad();
        log_info(logger, "cola elegida:%i",cola->prioridad);
        TCB *tcb = desencolar_multinivel(cola);
        tcb_en_ejecucion = tcb;
        log_info(logger,"Planificando TCB %d,PID %d con algoritmo MULTINIVEL\n", tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid);
        cambiar_estado_hilo(tcb, EXEC);
        enviar_exec_a_cpu(tcb->tid,tcb->pcb_pid);

    }

}


void fin_de_quantum(){
    // habria q esperar a que cpu empiece a ejecutar?
    sleep(quantum);
    enviar_fin_quantum(tcb_en_ejecucion->tid,tcb_en_ejecucion->pcb_pid);
    //TODO mandar interrupt a cpu
}

void printear_colas_y_prioridades(){
    printf("Colas de prioridad\n");
    for (unsigned int i = 0; i < list_size(colas_prioridades); i++){
        COLA_PRIORIDAD* cola_prioridad = list_get(colas_prioridades, i);
        printf("Cola de prioridad: %i\n", cola_prioridad->prioridad);
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