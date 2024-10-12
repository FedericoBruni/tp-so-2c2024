#include "mensajeria.h"

extern int fd_memoria;
extern int cliente_fd_dispatch;

crear_proceso(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo){
    t_buffer *buffer = crear_buffer();
    cargar_string_al_buffer(buffer,archivo_de_instrucciones);
    cargar_int_al_buffer(buffer,tamanio_proceso);
    cargar_int_al_buffer(buffer,prio_hilo);
    t_paquete *paquete = crear_paquete(SOL_CREAR_PROCESO, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
}