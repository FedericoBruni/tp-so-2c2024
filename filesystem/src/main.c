#include "utilidades.h"
#include <signal.h>

extern t_log *logger;
extern char *puerto;
int cliente_fd;

int main(int argc, char *argv[])
{
    
    iniciar_filesystem();
    signal(SIGINT, terminar_ejecucion);
    int server_fd = iniciar_servidor(logger, puerto);

    

    while(1){
        int cliente_fd = accept(server_fd,NULL,NULL);
        if (cliente_fd == -1)
	    {
		    log_error(logger, "Error al aceptar cliente: %s\n", "Memoria");
		    exit(EXIT_FAILURE);
	    }
        log_info(logger, "## Memoria Conectado - FD del socket: %d", cliente_fd);

        pthread_t hilo_fs_memoria;
        pthread_create(&hilo_fs_memoria, NULL, (void *)escuchar_mensajes_memoria, cliente_fd);
        pthread_detach(hilo_fs_memoria);
    }


    //terminar_ejecucion(server_fd);

    return 0;
}
