#include "instrucciones.h"
extern CONTEXTO_CPU *contexto_en_ejecucion;
extern t_log *logger;
extern int cliente_fd_dispatch;
extern int fd_memoria;
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
extern char* rta_mutex_lock;


void SET(char *registro, uint32_t valor){
    uint32_t *reg = obtenerRegistro(registro);

    *reg = valor;
}

// Lee el valor de memoria correspondiente a la dirección física obtenida a partir de la Dirección Lógica 
// que se encuentra en el Registro Dirección y lo almacena en el Registro Datos.
void write_mem(char* registroDireccion, char* registroDatos){
    int dir_fisica = calcular_direccion_fisica(contexto_en_ejecucion->contexto_proceso, registroDireccion);
    log_trace(logger, "Dir física: %i", dir_fisica);
    if (dir_fisica == -1) {
        log_error(logger, "ERROR");
        return; // en realidad hay q hacer lo del segmentation fault
    }
    int valor = *obtenerRegistro(registroDatos);
    enviar_write_mem(valor, dir_fisica);
    switch(recibir_operacion(fd_memoria)){
        case WRITE_MEM_RTA:
            log_trace(logger, "Dato escrito");
            break;
        default:
            log_error(logger,"Error, codigo de operacion desconocido");
            break;
    }

    // contenido = leer_dir_fisica(DF)
    // obtener_registro(registroDatos) = contenido
    // las direcciones lógicas son el desplazamiento dentro de la partición en la que se encuentra el proceso.
    // las direcciones físicas se generarán como: [Base + desplazamiento]
    // Se debe validar que las solicitudes se encuentren dentro de la partición asignada,  es decir que sea menor al límite de la 
    // partición. De fallar dicha validación, ocurrirá un “Segmentation Fault”, en cuyo caso, se deberá actualizar el contexto 
    // de ejecución en Memoria y devolver el Tid al Kernel con motivo de Segmentation Fault.

}


int deserializar_rta_read_mem(int fd_memoria) {
    t_buffer* buffer = recibir_buffer_completo(fd_memoria);
    return extraer_int_del_buffer(buffer);

}

void read_mem(char* registroDatos, char* registroDireccion){
    int dir_fisica = calcular_direccion_fisica(contexto_en_ejecucion->contexto_proceso, registroDireccion);
    log_trace(logger, "Dir física: %i", dir_fisica);
    if (dir_fisica == -1) {
        log_error(logger, "ERROR");
        return; // en realidad hay q hacer lo del segmentation fault
    }
    enviar_read_mem(dir_fisica);
    switch(recibir_operacion(fd_memoria)){
        case READ_MEM_RTA:
            int dato = deserializar_rta_read_mem(fd_memoria);
            *obtenerRegistro(registroDatos) = dato;
            log_trace(logger, "Dato leido: %i", dato);
            break;
        default:
            log_error(logger,"Error, codigo de operacion desconocido");
            break;
    }
}

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
    // esperar rta?extern sem_t sem_mutex_lockeado;
}

void io(int tiempo) {
    enviar_io(tiempo);
    actualizar_contexto(fd_memoria); // xq es syscall, pero lo necesitamos?
    // esperar rta?
    sem_wait(&sem_io_solicitada);
}

void PROCESS_CREATE(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo){
    crear_proceso(archivo_de_instrucciones,tamanio_proceso,prio_hilo);
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_proceso_creado);
    // switch(recibir_operacion(cliente_fd_dispatch)){
    //     case PROCESO_CREADO:
    //         break;
    //     default:
    //         log_error(logger,"Error, codigo de operacion desconocido");
    //         break;
    // }

    
}

void THREAD_CREATE (char* archivo_pseudocodigo, int prioridad) {
    crear_hilo(archivo_pseudocodigo, prioridad);
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_hilo_creado);
}

void THREAD_JOIN (int tid) {
    thread_join(tid);
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_join_hilo);
}

void THREAD_CANCEL (int tid, int pid) {
    thread_cancel(tid, pid);
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_hilo_cancel);
}

void MUTEX_CREATE (char *recurso) {
    mutex_create(recurso);
    //sleep(5);
    actualizar_contexto(fd_memoria);

    sem_wait(&sem_mutex_creado);
    // switch(recibir_operacion(cliente_fd_dispatch)){
    //     case MUTEX_CREADO:
    //         log_error(logger, "Mutex creado correctamente");
    //         break;
    //     default:
    //         log_error(logger,"Error, codigo de operacion desconocido");
    //         break;
    // }
}

char* MUTEX_LOCK (char* recurso) {
    mutex_lock(recurso);
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_mutex_lockeado);
    return rta_mutex_lock;
}

void MUTEX_UNLOCK (char* recurso) {
    mutex_unlock(recurso);
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_mutex_unlockeado);
}

void THREAD_EXIT() {
    thread_exit();
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_thread_exit);
}

void PROCESS_EXIT() {
    process_exit();
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_process_exit);
}
