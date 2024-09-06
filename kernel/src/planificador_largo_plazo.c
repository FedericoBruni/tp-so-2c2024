#include "planificador_largo_plazo.h"
extern t_list* cola_new;
extern t_log* logger;
extern int fd_memoria;
extern char* archivo_pseudocodigo;
extern int tamanio_proceso;

void planificador_largo_plazo(void){
    
    printf("Ruta: %s\nTam: %i\n", archivo_pseudocodigo, tamanio_proceso);
    while (true) {
        //sem_wait(&sem_NEW);
        PCB* pcb = list_remove(cola_new, 0);
        if (list_is_empty(cola_new)){
            log_info(logger,"solicitando memoria");
            int resultado = solicitar_memoria(fd_memoria,tamanio_proceso,SOLICITAR_MEMORIA_PROCESO);
            switch(resultado){
                case 1:
                    log_info(logger, "Memoria reservada correctamente");
                    // crear TCB con TID = 0
                    // asignar el TCB al PCB INICIAL.
                    imprimir_pcb(pcb);
                    break;

                case 0:
                    log_warning(logger, "No hay espacio en memoria");
                    break;
            }
                
        }
        break; // por ahora, q lo haga una sola vez pq no tenemos el semaforo

    }

}