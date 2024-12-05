#include "escuchar_mensajes_memoria.h"
extern t_log *logger;
extern int cliente_fd_kernel;
extern int cliente_fd_dispatch;
extern t_list *contextos_procesos;
extern t_list *contextos_hilos;

void escuchar_mensajes_kernel(void)
{
    int desconexion = 0;
    t_buffer *buffer;
    while (!desconexion)
    {
        int cod_op = recibir_operacion(cliente_fd_kernel);
        int pid;
        switch (cod_op)
        {
        case HANDSHAKE_KERNEL_MEMORIA:
            aceptar_handshake(logger, cliente_fd_kernel, HANDSHAKE_KERNEL_MEMORIA);
            break;
        case SOLICITAR_MEMORIA_PROCESO: // respuestas: 	OK_SOLICITUD_MEMORIA_PROCESO, ERROR_SOLICITUD_MEMORIA_PROCESO
            buffer = recibir_buffer_completo(cliente_fd_kernel);
            pid = extraer_int_del_buffer(buffer);
            int tamanio = extraer_int_del_buffer(buffer);
            Particion* particion = buscar_particion(tamanio);
            if(particion != NULL){
                CONTEXTO_PROCESO* contexto_proceso = malloc(sizeof(CONTEXTO_PROCESO));
                contexto_proceso->pid = pid;
                contexto_proceso->BASE = particion->inicio;
                contexto_proceso->LIMITE = particion->inicio + particion->tamanio;
                list_add(contextos_procesos, contexto_proceso);
                int rta_sol_mem = OK_SOLICITUD_MEMORIA_PROCESO;
                send(cliente_fd_kernel, &rta_sol_mem, sizeof(op_code), 0);
                log_info(logger, "## Proceso Creado - PID: %d - Tamaño: %d",pid,particion->tamanio);
            } else {
                int rta_sol_mem_err = ERROR_CREACION_HILO;
                send(cliente_fd_kernel, &rta_sol_mem_err, sizeof(op_code), 0); 
            }
            break;
        case FINAL_PROCESO:
            buffer = recibir_buffer_completo(cliente_fd_kernel);
            pid = extraer_int_del_buffer(buffer);
            finalizacion_de_proceso(pid);
            int rta_fin_proc = OK_FINAL_PROCESO;
            send(cliente_fd_kernel, &rta_fin_proc, sizeof(op_code), 0);
            break;
        case SOLICITAR_CREACION_HILO:
            buffer = recibir_buffer_completo(cliente_fd_kernel);
            CONTEXTO_HILO *contexto_hilo = extraer_tcb_del_buffer(buffer);
            list_add(contextos_hilos,contexto_hilo);
            cargar_archivo(contexto_hilo->archivo_pseudocodigo, contexto_hilo->tid,contexto_hilo->pid);
            log_info(logger, "## Hilo Creado - (PID:TID)- %d:%d",contexto_hilo->pid,contexto_hilo->tid);
            int rta_crear_hilo = OK_CREACION_HILO;
            send(cliente_fd_kernel, &rta_crear_hilo,sizeof(op_code),0);
            break;
        case FINAL_HILO: 
            t_buffer *buffer_fin = recibir_buffer_completo(cliente_fd_kernel);
            int tid_a_finalizar = extraer_int_del_buffer(buffer_fin);
            int pid_del_hilo_fin = extraer_int_del_buffer(buffer_fin);

            log_info(logger, "## Hilo Destruido - (PID:TID)- %d:%d", tid_a_finalizar, pid_del_hilo_fin);

            eliminar_hilo_y_contexto(tid_a_finalizar, pid_del_hilo_fin);

            int rta_fin_hilo = OK_FINAL_HILO;
            send(cliente_fd_kernel, &rta_fin_hilo, sizeof(op_code), 0);

            break;
        case CANCELAR_HILO:
            t_buffer *buffer_cancel = recibir_buffer_completo(cliente_fd_kernel);
            int tid_a_cancelar = extraer_int_del_buffer(buffer);
            int pid_del_hilo_cancel = extraer_int_del_buffer(buffer);

            log_info(logger, "Finalizando hilo con TID: %i y PID: %i", tid_a_cancelar, pid_del_hilo_cancel);

            //eliminar_hilo_y_contexto(tid_a_cancelar, pid_del_hilo_cancel);

            int rta_cancel_hilo = OK_FINAL_HILO;
            send(cliente_fd_kernel, &rta_fin_hilo, sizeof(op_code), 0);

            break;
        case SOL_DUMP:
            t_buffer *buffer_dump = recibir_buffer_completo(cliente_fd_kernel);
            int tid_dump = extraer_int_del_buffer(buffer_dump);
            int pid_dump = extraer_int_del_buffer(buffer_dump);
            dump_memory(tid_dump,pid_dump);
            int res_dump = MEM_DUMPEADA;
	        send(cliente_fd_kernel, &res_dump, sizeof(int), 0);
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
        int cod_op = recibir_operacion(cliente_fd_dispatch);

        switch (cod_op)
        {
        case HANDSHAKE_CPU_MEMORIA:
            aceptar_handshake(logger, cliente_fd_dispatch, HANDSHAKE_CPU_MEMORIA);
            break;
        case SOLICITAR_CONTEXTO:
            enviar_contexto(cliente_fd_dispatch);
            break;
        case ACTUALIZAR_CONTEXTO:
            actualizar_contexto(cliente_fd_dispatch);
            break;
        case SOLICITAR_INSTRUCCION:
            enviar_instruccion(cliente_fd_dispatch);
            break;
        case WRITE_MEM:
            deserializar_write_mem(cliente_fd_dispatch);
            break;
        case READ_MEM:
            deserializar_read_mem(cliente_fd_dispatch);
            break;
        case -1:
            log_error(logger, "Cpu desconectado\n");
            cliente_fd_dispatch = -1;
            return;
        default:
            log_warning(logger, "Codigo de operacion invalido Cpu");
            break;
        }
    }
}