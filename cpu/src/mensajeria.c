#include "mensajeria.h"

extern int fd_memoria;
extern int cliente_fd_dispatch;
extern CONTEXTO_CPU *contexto_en_ejecucion;
extern pthread_mutex_t mutex_conexion_dispatch;
extern pthread_mutex_t mutex_conexion_memoria;

void crear_proceso(char *archivo_de_instrucciones, int tamanio_proceso, int prio_hilo)
{
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, archivo_de_instrucciones);
    cargar_int_al_buffer(buffer, tamanio_proceso);
    cargar_int_al_buffer(buffer, prio_hilo);
    t_paquete *paquete = crear_paquete(SYSCALL_PROCESS_CREATE, buffer);
    pthread_mutex_lock(&mutex_conexion_dispatch);
    enviar_paquete(paquete, cliente_fd_dispatch);
    pthread_mutex_unlock(&mutex_conexion_dispatch);
    eliminar_paquete(paquete);
}

void crear_hilo(char *archivo_pseudocodigo, int prioridad)
{
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, archivo_pseudocodigo);
    cargar_int_al_buffer(buffer, prioridad);
    t_paquete *paquete = crear_paquete(SYSCALL_THREAD_CREATE, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void thread_join(int tid_bloqueante)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid_bloqueante);
    t_paquete *paquete = crear_paquete(SYSCALL_THREAD_JOIN, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void thread_cancel(int tid, int pid)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(SYSCALL_THREAD_CANCEL, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void mutex_create(char *recurso)
{
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, recurso);
    t_paquete *paquete = crear_paquete(SYSCALL_MUTEX_CREATE, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void mutex_lock(char *recurso)
{
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, recurso);
    t_paquete *paquete = crear_paquete(SYSCALL_MUTEX_LOCK, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void mutex_unlock(char *recurso)
{
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, recurso);
    t_paquete *paquete = crear_paquete(SYSCALL_MUTEX_UNLOCK, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void thread_exit()
{
    int rta = SYSCALL_THREAD_EXIT;
    send(cliente_fd_dispatch, &rta, sizeof(op_code), 0);
}

void process_exit()
{
    int process_exit = SYSCALL_PROCESS_EXIT;
    send(cliente_fd_dispatch, &process_exit, sizeof(op_code), 0);
}

void enviar_dump_memory(int pid, int tid)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, pid);
    cargar_int_al_buffer(buffer, tid);
    t_paquete *paquete = crear_paquete(SYSCALL_DUMP_MEMORY, buffer);
    pthread_mutex_lock(&mutex_conexion_dispatch);
    enviar_paquete(paquete, cliente_fd_dispatch);
    pthread_mutex_unlock(&mutex_conexion_dispatch);
    eliminar_paquete(paquete);
}

void enviar_io(int tiempo)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tiempo);
    t_paquete *paquete = crear_paquete(SYSCALL_IO, buffer);
    pthread_mutex_lock(&mutex_conexion_dispatch);
    enviar_paquete(paquete, cliente_fd_dispatch);
    pthread_mutex_unlock(&mutex_conexion_dispatch);
    eliminar_paquete(paquete);
}

void actualizar_contexto(int fd_memoria)
{
    t_buffer *bufferCtx = crear_buffer();

    cargar_contexto_hilo(bufferCtx, contexto_en_ejecucion->contexto_hilo);
    cargar_contexto_proceso(bufferCtx, contexto_en_ejecucion->contexto_proceso);
    t_paquete *paquete = crear_paquete(ACTUALIZAR_CONTEXTO, bufferCtx);
    pthread_mutex_lock(&mutex_conexion_memoria);
    enviar_paquete(paquete, fd_memoria);
    pthread_mutex_unlock(&mutex_conexion_memoria);
    eliminar_paquete(paquete);

    switch (recibir_operacion(fd_memoria))
    {
    case CONTEXTO_ACTUALIZADO_OK:

        log_info(logger, "## (%d:%d) - Actualizo Contexto Ejecucion", contexto_en_ejecucion->contexto_hilo->pid, contexto_en_ejecucion->contexto_hilo->tid);
        break;
    case CONTEXTO_ACTUALIZADO_ERROR:
        printf("Ok\n");
        break;
    default:
        log_error(logger, "Error actualizando el contexto");
        break;
    }
}

void enviar_write_mem(int valor, int direccion_fisica)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, valor);
    cargar_int_al_buffer(buffer, direccion_fisica);
    t_paquete *paquete = crear_paquete(WRITE_MEM, buffer);
    pthread_mutex_lock(&mutex_conexion_memoria);
    enviar_paquete(paquete, fd_memoria);
    pthread_mutex_unlock(&mutex_conexion_memoria);
    eliminar_paquete(paquete);
}

void enviar_read_mem(int direccion_fisica)
{
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, direccion_fisica);
    t_paquete *paquete = crear_paquete(READ_MEM, buffer);
    pthread_mutex_lock(&mutex_conexion_memoria);
    enviar_paquete(paquete, fd_memoria);
    pthread_mutex_unlock(&mutex_conexion_memoria);
    eliminar_paquete(paquete);
}
