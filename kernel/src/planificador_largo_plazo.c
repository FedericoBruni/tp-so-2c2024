#include "planificador_largo_plazo.h"
extern t_list* cola_new;
extern t_log* logger;
extern int fd_memoria;

void planificador_largo_plazo(void){
    

    while (true) {
        //sem_wait(&sem_NEW);
        if (list_is_empty(cola_new)){
            log_info(logger,"solicitando memoria");
            int resultado = solicitar_memoria(fd_memoria,500,SOLICITAR_MEMORIA_PROCESO);
            switch(resultado){
                case 1:
                    log_info(logger, "Memoria reservada correctamente");
                    break;

                case 0:
                    log_warning(logger, "No hay espacio en memoria");
                    break;
            }
                
        }

    }

}