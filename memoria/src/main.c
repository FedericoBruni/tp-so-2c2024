#include "utilidades.h"
#include "signal.h"

extern t_log *logger;
extern char *puerto_escucha;
int cliente_fd_dispatch;
int cliente_fd_kernel;
int server_fd;

int main(int argc, char *argv[])
{

    iniciar_memoria();
    // Iniciar servidor
    server_fd = iniciar_servidor(logger, puerto_escucha);
    signal(SIGINT, terminar_ejecucion);
    // Conectarse a FS

    int fd_filesystem = conectarse_a_filesystem();
    if (realizar_handshake(logger, fd_filesystem, HANDSHAKE_MEMORIA_FS) == -1)
    {
        exit(EXIT_FAILURE);
    }


    cliente_fd_dispatch = esperar_cliente(server_fd, logger, "Cpu");
    pthread_t hilo_cpu_memoria;
    pthread_create(&hilo_cpu_memoria, NULL, (void *)escuchar_mensajes_cpu, NULL);
    pthread_detach(hilo_cpu_memoria);

    // cliente_fd_kernel = esperar_cliente(server_fd, logger, "Kernel");

    // pthread_t hilo_kernel_memoria;
    // pthread_create(&hilo_kernel_memoria, NULL, (void *)escuchar_mensajes_kernel, NULL);
    // pthread_detach(hilo_kernel_memoria);

    while(1){
        cliente_fd_kernel = esperar_cliente(server_fd, logger, "Kernel");
        if (cliente_fd_kernel == -1)
	    {
		    log_error(logger, "Error al aceptar Kernel");
		    exit(EXIT_FAILURE);
	    }
        
        log_info(logger, "## Kernel Conectado - FD del socket: %d", cliente_fd_kernel);

        pthread_t hilo_kernel_memoria;
        pthread_create(&hilo_kernel_memoria, NULL, (void *)escuchar_mensajes_kernel, NULL);
        pthread_detach(hilo_kernel_memoria);
    }


    // while (1)
    // {
    //     if (cliente_fd_dispatch == -1 && cliente_fd_kernel == -1)
    //     {
    //         terminar_ejecucion(server_fd, cliente_fd_dispatch, cliente_fd_kernel);
    //     }
    // }

    return 0;
}
