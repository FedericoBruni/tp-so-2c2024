#include "syscalls.h"

extern t_queue *cola_new;
extern t_queue *cola_ready;
extern t_queue *cola_finalizacion;
extern t_list *
extern t_log *logger;
extern t_list *procesos;
/*
PROCESS_CREATE, esta syscall recibirá 3 parámetros de la CPU, el primero será el nombre del archivo de pseudocódigo que
deberá ejecutar el proceso, el segundo parámetro es el tamaño del proceso en Memoria y el tercer parámetro es la prioridad
del hilo main (TID 0).
El Kernel c: 12
[INFO] 20:reará un nuevo PCB y un TCB asociado con TID 0 y lo dejará en estado NEW.
*/

void PROCESS_CREATE(char *archivo, int tamanio_memoria, int prioridad)
{
    PCB *pcb = crear_pcb(archivo, tamanio_memoria, prioridad);
    queue_push(cola_new, pcb);
    // signal
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
        log_warning(logger, "Este hilo no tiene los permisos para finalizar el proceso: %i", tcb->pcb_pid);
        return -1;
    }
    PCB *pcb = tcb->pcb;
    mover_tcbs_exit(pcb);
    queue_push(cola_finalizacion, pcb);
    // Faltaría avisarle a Memoria??
    // Y sacar el pcb de la cola en la que está?
    // singal final_proceso
}

void THREAD_CREATE(int pid,char* archivo_pseudocodigo, int prioridad){
    PCB *pcb_en_ejecucion = buscar_pcb_en_ejecucion(pid);
    TCB *tcb = crear_tcb(pcb_en_ejecucion,prioridad,archivo_pseudocodigo);
    cambiar_estado_hilo(tcb,READY);
    //señal d creacion
    // para mi se pone acá en ready y listo, por como dice el enunciado
    // Al momento de crear el nuevo hilo, deberá generar el nuevo TCB con un TID autoincremental y 
    // poner al mismo en el estado READY.
    //Dejo todo comentado pq no existe "pcb_en_ejecucion" y rompe
}


// Vamos a tener un pcb_en_ejecucion, y cada pcb tiene una ref al hilo que esta ejecutando en ese momento? o también 
// tener una variable tcb_en_ejecucion?