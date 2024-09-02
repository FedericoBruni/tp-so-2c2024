#include "utilidades.h"


extern t_log* logger;
extern char* puerto_escucha;


int main(int argc, char* argv[]) {
    iniciar_memoria();
    // Conectarse a FS

    int fd_filesystem = conectarse_a_filesystem();

    // Iniciar servidor
    int server_fd = iniciar_servidor(logger,puerto_escucha);
    int cliente_fd_cpu = esperar_cliente(server_fd, logger, "Cpu");
    int cliente_fd_kernel = esperar_cliente(server_fd, logger, "Kernel");


    //terminar_ejecucion(server_fd);
    return 0;
}

