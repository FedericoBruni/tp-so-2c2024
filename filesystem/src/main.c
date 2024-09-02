#include "utilidades.h"

extern t_log* logger;
extern char* puerto;

int main(int argc, char* argv[]) {
 
    iniciar_filesystem();
    int server_fd = iniciar_servidor(logger, puerto);
    int cliente_fd = esperar_cliente(server_fd, logger,"Memoria");



    

    terminar_ejecucion(server_fd);

    return 0;
}
