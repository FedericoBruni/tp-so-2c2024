#include "utilidades.h"

extern t_log *logger;
extern char *puerto;
int cliente_fd;

int main(int argc, char *argv[])
{

    iniciar_filesystem();
    int server_fd = iniciar_servidor(logger, puerto);
    cliente_fd = esperar_cliente(server_fd, logger, "Memoria");

    pthread_t hilo_fs_memoria;
    pthread_create(&hilo_fs_memoria, NULL, (void *)escuchar_mensajes_memoria, NULL);
    pthread_join(hilo_fs_memoria, NULL);

    terminar_ejecucion(server_fd);

    return 0;
}
