#include "main.h"

extern char* puerto_escucha_dispatch;
extern char* puerto_escucha_interrupt;
extern t_log* logger;
int cliente_fd_dispatch;
int cliente_fd_interrupt;

int main(int argc, char* argv[]) {
    iniciar_cpu();
    // Conectarse a FS

    int fd_memoria = conectarse_a_memoria();

    // Iniciar servidores
    int server_fd_dispatch = iniciar_servidor(logger,puerto_escucha_dispatch);
    int server_fd_interrupt = iniciar_servidor(logger,puerto_escucha_interrupt);
    
    cliente_fd_dispatch = esperar_cliente(server_fd_dispatch, logger,"Kernel - Dispatcher");
    cliente_fd_interrupt = esperar_cliente(server_fd_interrupt, logger,"Kernel - Interrupt");

    pthread_t hilo_kernel_dispatch;
	pthread_create(&hilo_kernel_dispatch, NULL, (void*)escuchar_mensajes_kernel_dispatch, NULL); // Crea el hilo y le pasa la funcion a ejecutarse
	pthread_detach(hilo_kernel_dispatch); // Hace que el hilo se desacople del principal y se ejecute en paralelo

    pthread_t hilo_kernel_interrupt;
    pthread_create(&hilo_kernel_interrupt, NULL, (void*)escuchar_mensajes_kernel_interrupt, NULL); // Crea el hilo y le pasa la funcion a ejecutarse
    pthread_join(hilo_kernel_insdterrupt, NULL); // Hace que el hilo se desacople del principal y se ejecute en paralelo
    
    while (1){ // faltaria algun otro hilo en join que bloquee a terminar_ejecucion
        if (cliente_fd_interrupt == -1 && cliente_fd_dispatch == -1)
            terminar_ejecucion(server_fd_dispatch,server_fd_interrupt,fd_memoria);
    }
    
    return 0;
}
