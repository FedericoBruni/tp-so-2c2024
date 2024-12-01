#include "instrucciones.h"
extern CONTEXTO_CPU *contexto_en_ejecucion;
extern t_log *logger;
extern int cliente_fd_dispatch;
extern int fd_memoria;
void SET(char *registro, uint32_t valor){
    uint32_t *reg = obtenerRegistro(registro);

    *reg = valor;
}

// Lee el valor de memoria correspondiente a la dirección física obtenida a partir de la Dirección Lógica 
// que se encuentra en el Registro Dirección y lo almacena en el Registro Datos.
void READ_MEM(char* registroDatos, char* registroDireccion){
    // DF = obtener_dir_fisica(registroDireccion)
    // contenido = leer_dir_fisica(DF)
    // obtener_registro(registroDatos) = contenido
    // las direcciones lógicas son el desplazamiento dentro de la partición en la que se encuentra el proceso.
    // las direcciones físicas se generarán como: [Base + desplazamiento]
    // Se debe validar que las solicitudes se encuentren dentro de la partición asignada,  es decir que sea menor al límite de la 
    // partición. De fallar dicha validación, ocurrirá un “Segmentation Fault”, en cuyo caso, se deberá actualizar el contexto 
    // de ejecución en Memoria y devolver el Tid al Kernel con motivo de Segmentation Fault.

}

// void WRITE MEM(){

// }

void SUM(char *registro_destino, char *registro_origen){
    uint32_t *destino = obtenerRegistro(registro_destino);
    uint32_t *origen = obtenerRegistro(registro_origen);
    *destino += *origen;
}

void SUB(char *registro_destino, char *registro_origen){
    uint32_t *destino = obtenerRegistro(registro_destino);
    uint32_t *origen = obtenerRegistro(registro_origen);

    *destino = *destino - *origen;
}

void JNZ(char *registro, uint32_t instruccion){
    uint32_t *reg = obtenerRegistro(registro);

    if(*reg != 0){
        contexto_en_ejecucion->contexto_hilo->Registros->PC = instruccion;
    }
}

void LOG(char *registro){
    uint32_t *reg = obtenerRegistro(registro);

    log_info(logger, "El valor del registro %s es: %u",registro,*reg);
}

void DUMP_MEMORY(int pid, int tid) {
    enviar_dump_memory(pid, tid);
    actualizar_contexto(fd_memoria);
    // esperar rta?
}

void IO (int tiempo) {
    enviar_io(tiempo);
    actualizar_contexto(fd_memoria); // xq es syscall, pero lo necesitamos?
    // esperar rta?
}

void PROCESS_CREATE(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo){
    crear_proceso(archivo_de_instrucciones,tamanio_proceso,prio_hilo);
    actualizar_contexto(fd_memoria);

    switch(recibir_operacion(cliente_fd_dispatch)){
        case PROCESO_CREADO:
            printf("Llego PROCESO_CREADO\n");
            break;
        default:
            log_error(logger,"Error, codigo de operacion desconocido");
            break;
    }

    
}

void THREAD_CREATE (char* archivo_pseudocodigo, int prioridad) {
    crear_hilo(archivo_pseudocodigo, prioridad);
    actualizar_contexto(fd_memoria);
}

void THREAD_JOIN (int tid) {
    thread_join(tid);
    actualizar_contexto(fd_memoria);
}

void THREAD_CANCEL (int tid, int pid) {
    thread_cancel(tid, pid);
    actualizar_contexto(fd_memoria);
}

void MUTEX_CREATE (char *recurso) {
    mutex_create(recurso);
    actualizar_contexto(fd_memoria);
}

void MUTEX_LOCK (char* recurso) {
    mutex_lock(recurso);
    actualizar_contexto(fd_memoria);
}

void MUTEX_UNLOCK (char* recurso) {
    mutex_unlock(recurso);
    actualizar_contexto(fd_memoria);
}

void THREAD_EXIT(int tid, int pid) {
    thread_exit(tid, pid);
    actualizar_contexto(fd_memoria);
}

void PROCESS_EXIT(int pid) {
    process_exit(pid);
    actualizar_contexto(fd_memoria);
}
