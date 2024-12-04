#include "escuchar_kernel.h"

extern t_log *logger;
extern int cliente_fd_dispatch;
extern int cliente_fd_interrupt;
extern sem_t sem_proceso_creado;
extern sem_t sem_mutex_creado;
extern sem_t sem_mutex_lockeado;
extern sem_t sem_mutex_unlockeado;
extern sem_t sem_thread_exit;
extern sem_t sem_process_exit;
extern sem_t sem_hilo_creado;
extern sem_t sem_join_hilo;
extern sem_t sem_hilo_cancel;
extern sem_t sem_io_solicitada;
char* rta_mutex_lock;
extern bool flag_interrupt;
extern pthread_mutex_t mutex_interrupt;

void escuchar_mensajes_kernel_dispatch(void)
{
	bool desconexion_kernel_dispatch = 0;

	while (!desconexion_kernel_dispatch)
	{
		int cod_op = recibir_operacion(cliente_fd_dispatch); // recv() es bloqueante por ende no queda loopeando infinitamente
															 // printf("---recibir_operacion--->cod_op= %d\n",cod_op);

		switch (cod_op)
		{
		case HANDSHAKE_KERNEL_CPU_DISPATCH:
			aceptar_handshake(logger, cliente_fd_dispatch, HANDSHAKE_KERNEL_CPU_DISPATCH);
			break;

		case ENVIAR_EXEC:
			recibir_exec(logger,cliente_fd_dispatch,cod_op);
			break;

		case PROCESO_CREADO:
			sem_post(&sem_proceso_creado);
			break;
		case HILO_CREADO:
			sem_post(&sem_hilo_creado);
			break;	
		case HILO_JOINEADO:
			sem_post(&sem_join_hilo);
			break;
		case HILO_CANCEL:
			sem_post(&sem_hilo_cancel);
			break;
		case MUTEX_CREADO:
			sem_post(&sem_mutex_creado);
			break;
		case MUTEX_LOCKEADO:
			rta_mutex_lock = "OK";
			sem_post(&sem_mutex_lockeado);
			break;
		case MUTEX_UNLOCKEADO:
			sem_post(&sem_mutex_unlockeado);
			break;	

		case LOCKEAR_HILO:
			rta_mutex_lock = "SUSPPROCESO";
			sem_post(&sem_mutex_lockeado);
			break;
		case FIN_HILO:
			sem_post(&sem_thread_exit);
			break;	
		case FIN_PROCESO:
			sem_post(&sem_process_exit);
			break;
		case IO_SOLICITADA:
			sem_post(&sem_io_solicitada);
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

void escuchar_mensajes_kernel_interrupt(void)
{
	bool desconexion_kernel_interrupt = 0;
	while (!desconexion_kernel_interrupt)
	{
		int cod_op = recibir_operacion(cliente_fd_interrupt); // recv() es bloqueante por ende no queda loopeando infinitamente
		switch (cod_op)
		{
		case HANDSHAKE_KERNEL_CPU_INTERRUPT:
			aceptar_handshake(logger, cliente_fd_interrupt, HANDSHAKE_KERNEL_CPU_INTERRUPT);
			break;
		case FIN_QUANTUM:
			pthread_mutex_lock(&mutex_interrupt);
			flag_interrupt = true;
			pthread_mutex_unlock(&mutex_interrupt);
		    //procesar_fin_quantum(logger, cliente_fd_interrupt,cod_op);
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

