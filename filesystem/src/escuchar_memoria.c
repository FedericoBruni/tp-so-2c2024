#include"escuchar_memoria.h"

extern t_log* logger;
extern int cliente_fd;

void escuchar_mensajes_memoria(){
    int desconexion = 0;
    while(!desconexion){
        int cod_op = recibir_operacion(cliente_fd);
        
        switch(cod_op){
            case HANDSHAKE_MEMORIA_FS:
                aceptar_handshake(logger,cliente_fd, HANDSHAKE_MEMORIA_FS);
                break;
            case -1:
            	log_error(logger, "Memoria desconectado\n");
				cliente_fd = -1;
				return;
            default:
                log_warning(logger, "Codigo de operacion invalido");
				break; 
        }
    }
}