#include "utilidades.h"

extern t_log* logger;
int fd_memoria;

int main(int argc, char* argv[]) {

    iniciar_kernel();
    
    // Conectarse a memoria
    fd_memoria = conectarse_a_memoria();
    // Conectarse a cpu dispatch
    int fd_cpu_dispatch = conectarse_a_cpu_dispatch();
    // Conectarse a cpu interrupt
    int fd_cpu_interrupt = conectarse_a_cpu_interrupt();

    // Handshakes
    if (realizar_handshake(logger, fd_memoria, HANDSHAKE_KERNEL_MEMORIA) == -1){
        exit(EXIT_FAILURE);
    }
    if (realizar_handshake(logger, fd_cpu_dispatch, HANDSHAKE_KERNEL_CPU_DISPATCH) == -1){
        exit(EXIT_FAILURE);
    }
    if (realizar_handshake(logger, fd_cpu_interrupt, HANDSHAKE_KERNEL_CPU_INTERRUPT) == -1){
        exit(EXIT_FAILURE);
    }
    
    pthread_t hilo_planificador_largo_plazo;
    pthread_create(&hilo_planificador_largo_plazo, NULL, (void *)planificador_largo_plazo, NULL); // Crea el hilo y le pasa la funcion a ejecutarse
    pthread_join(hilo_planificador_largo_plazo, NULL);       

    terminar_ejecucion(fd_cpu_dispatch,fd_memoria,fd_cpu_interrupt);

    return 0;


}



