#include "utilidades.h"

extern t_log* logger;

int main(int argc, char* argv[]) {

    iniciar_kernel();
    
    // Conectarse a memoria
    int fd_memoria = conectarse_a_memoria();
    // Conectarse a cpu dispatch
    int fd_cpu_dispatch = conectarse_a_cpu_dispatch();
    
    // Conectarse a cpu interrupt
    int fd_cpu_interrupt = conectarse_a_cpu_interrupt();

    if (realizar_handshake(logger, fd_cpu_dispatch, HANDSHAKE_KERNEL_DISPATCH) == -1){
        exit(EXIT_FAILURE);
    }
    if (realizar_handshake(logger, fd_cpu_interrupt, HANDSHAKE_KERNEL_INTERRUPT) == -1){
        exit(EXIT_FAILURE);
    }
    
    
    terminar_ejecucion(fd_cpu_dispatch,fd_memoria,fd_cpu_interrupt);

    return 0;


}



