#include "escuchar_kernel.h"

extern t_log* logger;
extern int cliente_fd_dispatch;
extern int cliente_fd_interrupt;

void escuchar_mensajes_kernel_dispatch(void){
    bool desconexion_kernel_dispatch = 0;

	while(!desconexion_kernel_dispatch){
		int cod_op = recibir_operacion(cliente_fd_dispatch); // recv() es bloqueante por ende no queda loopeando infinitamente
			//printf("---recibir_operacion--->cod_op= %d\n",cod_op);

		switch(cod_op){
			case HANDSHAKE_KERNEL_DISPATCH:
				aceptar_handshake(logger, cliente_fd_dispatch, "dispatch");
				break;

			case -1:
				log_error(logger, "Dispatch desconectado\n");
				cliente_fd_dispatch = -1;
				return;

			default:
				log_warning(logger, "Codigo de operacion invalido dispatch");
				break;
		}
	}
}

void escuchar_mensajes_kernel_interrupt(void){
    bool desconexion_kernel_interrupt = 0;
	while(!desconexion_kernel_interrupt){
		int cod_op = recibir_operacion(cliente_fd_interrupt); // recv() es bloqueante por ende no queda loopeando infinitamente
		switch(cod_op){
			case HANDSHAKE_KERNEL_INTERRUPT:
				aceptar_handshake(logger, cliente_fd_interrupt, "interrupt");
				break;

			case -1:
				log_error(logger, "Interrupt desconectado\n");
				cliente_fd_interrupt = -1;
				return;

			default:
				log_warning(logger, "Codigo de operacion invalido interrupt");
				break;
		}
	}
}