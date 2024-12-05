#include "escuchar_memoria.h"

extern t_log *logger;
//extern int cliente_fd;

void escuchar_mensajes_memoria(int cliente_fd)
{
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op)
        {
        case HANDSHAKE_MEMORIA_FS:
            aceptar_handshake(logger, cliente_fd, HANDSHAKE_MEMORIA_FS);
            log_trace(logger,"TEST HANDSHAKE");
            break;
        case SOL_DUMP:
            t_buffer *buffer_dump = recibir_buffer_completo(cliente_fd);
            int tid = extraer_int_del_buffer(buffer_dump);
            int pid = extraer_int_del_buffer(buffer_dump);
            int tamanio = extraer_int_del_buffer(buffer_dump);
            char *contenido = extraer_string_del_buffer(buffer_dump);
            crear_archivo(pid,tid,tamanio,contenido);
            int result_dump = MEM_DUMPEADA;
	        send(cliente_fd, &result_dump, sizeof(int), 0);
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