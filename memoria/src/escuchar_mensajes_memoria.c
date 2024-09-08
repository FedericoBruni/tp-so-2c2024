#include "escuchar_mensajes_memoria.h"
extern t_log *logger;
extern int cliente_fd_kernel;
extern int cliente_fd_cpu;

void escuchar_mensajes_kernel(void)
{
    int desconexion = 0;
    t_buffer *buffer;
    while (!desconexion)
    {
        int cod_op = recibir_operacion(cliente_fd_kernel);

        switch (cod_op)
        {
        case HANDSHAKE_KERNEL_MEMORIA:
            aceptar_handshake(logger, cliente_fd_kernel, HANDSHAKE_KERNEL_MEMORIA);
            break;
        case SOLICITAR_MEMORIA_PROCESO: // respuestas: 	OK_SOLICITUD_MEMORIA_PROCESO, ERROR_SOLICITUD_MEMORIA_PROCESO
            buffer = recibir_buffer_completo(cliente_fd_kernel);
            int tamanio = extraer_int_del_buffer(buffer);
            log_info(logger, "Tamanio de memoria a reservar: %i", tamanio);
            int rta_sol_mem = OK_SOLICITUD_MEMORIA_PROCESO;
            send(cliente_fd_kernel, &rta_sol_mem, sizeof(op_code), 0);
            break;
        case FINAL_PROCESO:
            buffer = recibir_buffer_completo(cliente_fd_kernel);
            int pid = extraer_int_del_buffer(buffer);
            log_info(logger, "Finalizando proceso con id: %i", pid);
            int rta_fin_proc = OK_FINAL_PROCESO;
            send(cliente_fd_kernel, &rta_fin_proc, sizeof(op_code), 0);
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

void escuchar_mensajes_cpu(void)
{
    int desconexion = 0;
    while (!desconexion)
    {
        int cod_op = recibir_operacion(cliente_fd_cpu);

        switch (cod_op)
        {
        case HANDSHAKE_CPU_MEMORIA:
            aceptar_handshake(logger, cliente_fd_cpu, HANDSHAKE_CPU_MEMORIA);
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