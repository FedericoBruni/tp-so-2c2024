#include "escuchar_mensajes_memoria.h"
extern t_log* logger;
extern int cliente_fd_kernel;
extern int cliente_fd_cpu;

void escuchar_mensajes_kernel(void){
    int desconexion = 0;
    while(!desconexion){
        int cod_op = recibir_operacion(cliente_fd_kernel);
        
        switch(cod_op){
            case HANDSHAKE_KERNEL:
                aceptar_handshake(logger,cliente_fd_kernel,"Kernel");
                break;
            case -1:
            	log_error(logger, "Kernel desconectado\n");
				cliente_fd_kernel = -1;
				return;
            default:
                log_warning(logger, "Codigo de operacion invalido Kernel");
				break; 
        }
    }
}

void escuchar_mensajes_cpu(void){
        int desconexion = 0;
    while(!desconexion){
        int cod_op = recibir_operacion(cliente_fd_cpu);
        
        switch(cod_op){
            case HANDSHAKE_CPU:
                aceptar_handshake(logger,cliente_fd_cpu,"Cpu");
                break;
            case -1:
            	log_error(logger, "Cpu desconectado\n");
				cliente_fd_cpu = -1;
				return;
            default:
                log_warning(logger, "Codigo de operacion invalido Cpu");
				break; 
        }
    }
}