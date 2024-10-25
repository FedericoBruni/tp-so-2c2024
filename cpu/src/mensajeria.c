#include "mensajeria.h"

extern int fd_memoria;
extern int cliente_fd_dispatch;
extern CONTEXTO_CPU *contexto_en_ejecucion;

void crear_proceso(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo){
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer,archivo_de_instrucciones);
    cargar_int_al_buffer(buffer,tamanio_proceso);
    cargar_int_al_buffer(buffer,prio_hilo);
    t_paquete *paquete = crear_paquete(SYSCALL_PROCESS_CREATE, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void crear_hilo(char* archivo_pseudocodigo, int prioridad) {
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, archivo_pseudocodigo);
    cargar_int_al_buffer(buffer, prioridad);
    t_paquete *paquete = crear_paquete(SYSCALL_THREAD_CREATE, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void thread_join(int tid) {
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    t_paquete *paquete = crear_paquete(SYSCALL_THREAD_JOIN, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void thread_cancel(int tid, int pid) {
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(SYSCALL_THREAD_CANCEL, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void mutex_create(char* recurso) {
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, recurso);
    t_paquete *paquete = crear_paquete(SYSCALL_MUTEX_CREATE, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void mutex_lock(char* recurso) {
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, recurso);
    t_paquete *paquete = crear_paquete(SYSCALL_MUTEX_LOCK, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void mutex_unlock(char* recurso) {
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer, recurso);
    t_paquete *paquete = crear_paquete(SYSCALL_MUTEX_UNLOCK, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void thread_exit(int tid, int pid) {
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tid);
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(SYSCALL_THREAD_EXIT, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void process_exit(int pid) {
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(SYSCALL_PROCESS_EXIT, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}

void actualizar_contexto(int fd_memoria){
    t_buffer *bufferCtx = crear_buffer();

    cargar_contexto_hilo(bufferCtx, contexto_en_ejecucion->contexto_hilo);
    cargar_contexto_proceso(bufferCtx,contexto_en_ejecucion->contexto_proceso);
    t_paquete *paquete = crear_paquete(ACTUALIZAR_CONTEXTO, bufferCtx);
    enviar_paquete(paquete,fd_memoria);
    eliminar_paquete(paquete);
    
}
