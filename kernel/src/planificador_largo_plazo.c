#include "planificador_largo_plazo.h"
extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_log* logger;
extern int fd_memoria;
extern char* archivo_pseudocodigo;
extern int tamanio_proceso;

void creacion_de_procesos(void){
    
    printf("Ruta: %s\nTam: %i\n", archivo_pseudocodigo, tamanio_proceso);
    while (true) {
        //sem_wait(&sem_NEW);
        PCB* pcb = queue_pop(cola_new);
        if (queue_is_empty(cola_new)){
            log_info(logger,"solicitando memoria");
            int resultado = solicitar_memoria(fd_memoria,tamanio_proceso,SOLICITAR_MEMORIA_PROCESO);
            switch(resultado){
                case 1:
                    log_info(logger, "Memoria reservada correctamente");
                    TCB* tcb = crear_tcb(pcb, pcb->prioridad_main);
                    agregar_hilo(tcb,pcb);
                    cambiar_estado_hilo(tcb,READY); // el estado del proceso depende del estado de los hilos, hay q ver como implementar eso
                    queue_push(cola_ready,pcb); //los hilos van en la cola de ready o los procesos? o ambos por separado
                    imprimir_pcb(pcb);
                    // liberar_pcb(pcb);
                    // if(pcb != NULL){
                    //     log_error(logger,"aaaaaaa");
                    // } habria que liberar el pcb que creamos o no? 
                    //si hay que borrar hay q fijarse las funciones d borrar pq andan medio mal
                    break;

                case 0:
                    log_warning(logger, "No hay espacio en memoria");
                    queue_push(cola_new,pcb);
                    //Habria que agregar mas logica aca? 
                    break;
            }
                
        }
        break; // por ahora, q lo haga una sola vez pq no tenemos el semaforo

    }

}

void finalizacion_de_procesos(void){
    
}
