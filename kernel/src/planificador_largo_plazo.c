#include "planificador_largo_plazo.h"
extern t_queue *cola_new;
extern t_queue *cola_ready;
extern t_queue *cola_finalizacion;
extern t_log *logger;
extern t_list *pcbs_en_ejecucion;
extern int fd_memoria;
extern char *archivo_pseudocodigo;
extern int tamanio_proceso;
extern bool hilo_creacion_muerto;
extern bool hilo_fin_proc_muerto;

void creacion_de_procesos(void)
{
    hilo_creacion_muerto = false;
    printf("Ruta: %s\nTam: %i\n", archivo_pseudocodigo, tamanio_proceso);
    while (true)
    {
        // sem_wait(&sem_NEW);
        PCB *pcb = queue_pop(cola_new);
        if (queue_is_empty(cola_new))
        {
            log_info(logger, "solicitando memoria");
            int resultado = solicitar_memoria(fd_memoria, tamanio_proceso, SOLICITAR_MEMORIA_PROCESO);
            switch (resultado)
            {
            case 1:
                log_info(logger, "Memoria reservada correctamente");
                list_add(pcbs_en_ejecucion,pcb);
                TCB *tcb = crear_tcb(pcb, pcb->prioridad_main, archivo_pseudocodigo);
                agregar_hilo(tcb, pcb);
                cambiar_estado_hilo(tcb, READY); // el estado del proceso depende del estado de los hilos, hay q ver como implementar eso
                queue_push(cola_ready, tcb);     // los hilos van en la cola de ready o los procesos? o ambos por separado
                imprimir_pcb(pcb);
                // liberar_pcb(pcb);
                // if(pcb != NULL){
                //     log_error(logger,"aaaaaaa");
                // } habria que liberar el pcb que creamos o no?
                // si hay que borrar hay q fijarse las funciones d borrar pq andan medio mal
                PROCESS_EXIT(tcb); // agrego para probar la finalizacion :) dsps hay q borrarlo

                break;

            case 0:
                log_warning(logger, "No hay espacio en memoria");
                queue_push(cola_new, pcb);
                // Habria que agregar mas logica aca?
                break;
            }
        }
        break; // por ahora, q lo haga una sola vez pq no tenemos el semaforo
    }
    hilo_creacion_muerto = true;
}

void finalizacion_de_procesos(void)
{
    hilo_fin_proc_muerto = false;
    while (true)
    {
        // sign_wait final_proceso
        PCB *pcb = queue_pop(cola_finalizacion);
        log_info(logger, "intentando finalizar proceso con pid: %i", pcb->pid);
        int resultado = notificar_finalizacion_proceso(fd_memoria, pcb->pid, FINAL_PROCESO);
        switch (resultado)
        {
        case 1:
            log_info(logger, "Proceso %i Finalizado con exito\n", pcb->pid);
            // liberar_pcb(pcb);
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
        //sig_wait creacion
        
    }
}
