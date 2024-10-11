#include "escuchar_mensajes_memoria.h"
extern t_log *logger;
extern int cliente_fd_kernel;
extern int cliente_fd_cpu;
extern t_list *contextos_procesos;
extern t_list *contextos_hilos;

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
            CONTEXTO_PROCESO* contexto_proceso = extraer_pcb_del_buffer(buffer);
            list_add(contextos_procesos, contexto_proceso);
            printf("Contexto recibido\nPID: %i\nBASE: %i\nLIMITE: %i\n", contexto_proceso->pid, contexto_proceso->BASE, contexto_proceso->LIMITE);
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
        case SOLICITAR_CREACION_HILO:
            buffer = recibir_buffer_completo(cliente_fd_kernel);
            CONTEXTO_HILO *contexto_hilo = extraer_tcb_del_buffer(buffer);
            list_add(contextos_hilos,contexto_hilo);
            log_info(logger, "Creando hilo con id: %i", contexto_hilo->tid);
            int rta_crear_hilo = OK_CREACION_HILO;
            send(cliente_fd_kernel, &rta_crear_hilo,sizeof(op_code),0);
            break;
        case FINAL_HILO:
            buffer = recibir_buffer_completo(cliente_fd_kernel);
            int tid_a_finalizar = extraer_int_del_buffer(buffer);
            int pid_del_hilo = extraer_int_del_buffer(buffer);
            log_info("Finalizando hilo con tid: %i", tid_a_finalizar);
            
            
            CONTEXTO_CPU *contexto_cpu = buscar_contextos(tid_a_finalizar,pid_del_hilo);
            if(contexto_cpu->contexto_hilo != NULL && contexto_cpu->contexto_proceso != NULL){
                int rta_fin_hilo = OK_FINAL_HILO;
                send(cliente_fd_kernel, &rta_fin_hilo, sizeof(op_code), 0);
            }

            break;
        case CANCELAR_HILO:
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
        case SOLICITAR_CONTEXTO:
            printf("Contexto solicitado\n");
            enviar_contexto(cliente_fd_cpu);
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