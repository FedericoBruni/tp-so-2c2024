#include "utilidades.h"


extern t_log* logger;
extern char* puerto_escucha;
int cliente_fd_cpu;
int cliente_fd_kernel;


int main(int argc, char* argv[]) {

    iniciar_memoria();
    // Iniciar servidor
    int server_fd = iniciar_servidor(logger,puerto_escucha);
    // Conectarse a FS

    int fd_filesystem = conectarse_a_filesystem();
    if (realizar_handshake(logger, fd_filesystem, HANDSHAKE_MEMORIA) == -1){
        exit(EXIT_FAILURE);
    }

    cliente_fd_cpu = esperar_cliente(server_fd, logger, "Cpu");
    cliente_fd_kernel = esperar_cliente(server_fd, logger, "Kernel");

    pthread_t hilo_kernel_memoria;
    pthread_create(&hilo_kernel_memoria,NULL,(void*)escuchar_mensajes_kernel,NULL);
    pthread_detach(hilo_kernel_memoria);

    pthread_t hilo_cpu_memoria;
    pthread_create(&hilo_cpu_memoria,NULL,(void*)escuchar_mensajes_cpu,NULL);
    pthread_join(hilo_cpu_memoria,NULL);

    while(1){
        if(cliente_fd_cpu == -1 && cliente_fd_kernel == -1){
            terminar_ejecucion(server_fd,cliente_fd_cpu,cliente_fd_kernel);
        }
    }




    return 0;
}

