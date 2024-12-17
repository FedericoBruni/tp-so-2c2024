#include "planificador_corto_plazo.h"

extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_blocked;
extern char *algoritmo_planificacion;
extern t_queue *cola_ready;
extern t_queue *cola_blocked;
extern TCB *tcb_en_ejecucion;
extern t_log *logger;
extern int fd_cpu_interrupt;
extern int fd_cpu_dispatch;
extern sem_t sem_hay_ready;
extern sem_t sem_cpu_ejecutando;
extern t_list *colas_prioridades;
extern int quantum;
extern PCB *pcb_en_ejecucion;
extern sem_t sem_puede_ejecutar;
extern sem_t sem_exec_recibido;
extern TCB *tcb_anterior;
extern bool fin_ciclo;

void planificador_corto_plazo()
{

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if (string_equals_ignore_case(algoritmo_planificacion, "FIFO"))
    {
        fifo();
    }
    else if (string_equals_ignore_case(algoritmo_planificacion, "PRIORIDADES"))
    {
        prioridad();
    }
    else if (string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL"))
    {
        multinivel();
    }
    else
    {
        // no existe el algoritmo
        log_error(logger, "No se encontro el algoritmo de planificacion solicitado");
        exit(EXIT_FAILURE);
    }
}

void fifo()
{
    while (1)
    {
        sem_wait(&sem_hay_ready);
        if(fin_ciclo) return;
        sem_wait(&sem_puede_ejecutar);
        TCB *tcb = desencolar(cola_ready, mutex_ready);
        if (tcb != NULL)
        {
            tcb_en_ejecucion = tcb;
            log_info(logger, "## Planificando (PID:TID) - (<%d>:<%d>)- Algoritmo <FIFO>", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            cambiar_estado_hilo(tcb, EXEC);
            pcb_en_ejecucion = tcb_en_ejecucion->pcb;
            enviar_exec_a_cpu(tcb->tid, tcb->pcb_pid);
            if (esperar_respuesta() == 1)
            {
                continue;
            }
        }
    }
}

void prioridad()
{
    while (1)
    {
        sem_wait(&sem_hay_ready);
        if(fin_ciclo) return;
        sem_wait(&sem_puede_ejecutar);
        ordenar_cola(cola_ready, mutex_ready);
        TCB *tcb = desencolar(cola_ready, mutex_ready);
        if (tcb != NULL)
        {
            tcb_en_ejecucion = tcb;
            log_info(logger, "## Planificando (PID:TID) - (<%d>:<%d>)- Algoritmo <PRIORIDADES>", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            cambiar_estado_hilo(tcb, EXEC);
            pcb_en_ejecucion = tcb_en_ejecucion->pcb;
            enviar_exec_a_cpu(tcb->tid, tcb->pcb_pid);
            esperar_respuesta();
        }
    }
}

void multinivel()
{

    while (1)
    {
        sem_wait(&sem_hay_ready);
        if(fin_ciclo) return;
        sem_wait(&sem_puede_ejecutar);
        COLA_PRIORIDAD *cola = obtener_cola_con_mayor_prioridad();
        int cant_empty = 0;
        for(int i = 0; i<list_size(colas_prioridades);i++){
            COLA_PRIORIDAD *cola_prio = list_get(colas_prioridades,i);
            if(queue_is_empty(cola_prio->cola_prioridad)){
                cant_empty++;
            }
        }
        if(cant_empty == list_size(colas_prioridades)) continue;
        TCB *tcb = desencolar_multinivel(cola);
        if (tcb != NULL)
        {
            tcb_en_ejecucion = tcb;
            log_info(logger, "## Planificando (PID:TID) - (<%d>:<%d>)- Algoritmo <MULTINIVEL>", tcb_en_ejecucion->pcb_pid, tcb_en_ejecucion->tid);
            cambiar_estado_hilo(tcb, EXEC);
            pcb_en_ejecucion = tcb_en_ejecucion->pcb;

            test *testStruct = malloc(sizeof(test));
            testStruct->pid = tcb_en_ejecucion->pcb_pid;
            testStruct->tid = tcb_en_ejecucion->tid;

            pthread_t hilo_contador;
            pthread_create(&hilo_contador, NULL, (void *)fin_de_quantum, testStruct);
            pthread_detach(hilo_contador);
            enviar_exec_a_cpu(tcb->tid, tcb->pcb_pid);
            sem_wait(&sem_exec_recibido);
            esperar_respuesta();
            //pthread_cancel(hilo_contador);
            free(testStruct);
        }
    }
}

void fin_de_quantum(test *str)
{
    int quantumCola = quantum;
    int tid = str->tid;
    int pid = str->pid;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    sem_wait(&sem_cpu_ejecutando);
    usleep(quantumCola * 1000);
    enviar_fin_quantum(tid, pid);
}

void printear_colas_y_prioridades()
{
    for (unsigned int i = 0; i < list_size(colas_prioridades); i++)
    {
        COLA_PRIORIDAD *cola_prioridad = list_get(colas_prioridades, i);
        log_trace(logger, "Cola prioridad: %i, Cola size: %i", cola_prioridad->prioridad, queue_size(cola_prioridad->cola_prioridad));
    }
}

int comparar_prioridades(TCB *tcb1, TCB *tcb2)
{
    return tcb1->prioridad <= tcb2->prioridad;
}

/*
int comparar_prioridades(void* a, void* b) {
    TCB* tcb1 = (TCB*)a;
    TCB* tcb2 = (TCB*)b;
    return tcb1->prioridad <= tcb2->prioridad;
}
*/