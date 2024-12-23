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
extern sem_t sem_dump_mem;
extern char* rta_mutex_lock;
extern char* rta_hilo_join;
extern sem_t sem_ctx_actualizado;
extern char* rta_mem_dump;



void SET(char *registro, uint32_t valor){
    uint32_t *reg = obtenerRegistro(registro);
    *reg = valor;
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
}

// Lee el valor de memoria correspondiente a la dirección física obtenida a partir de la Dirección Lógica 
// que se encuentra en el Registro Dirección y lo almacena en el Registro Datos.
int write_mem(char* registroDireccion, char* registroDatos){
    int dir_fisica = calcular_direccion_fisica(contexto_en_ejecucion->contexto_proceso, registroDireccion);
    if (dir_fisica == -1) {
        actualizar_contexto(fd_memoria);
        sem_wait(&sem_ctx_actualizado);
        segmentation_fault();
        sem_wait(&sem_process_exit);
        return 0; // en realidad hay q hacer lo del segmentation fault
    }



    int valor = *obtenerRegistro(registroDatos);
    enviar_write_mem(valor, dir_fisica);
    switch(recibir_operacion(fd_memoria)){
        case WRITE_MEM_RTA:
            contexto_en_ejecucion->contexto_hilo->Registros->PC++;
            break;
        default:
            log_error(logger,"Error desconocido escribiendo memoria");
            break;
    }
    return 1;
}


int deserializar_rta_read_mem(int fd_memoria) {
    t_buffer* buffer = recibir_buffer_completo(fd_memoria);
    int rta = extraer_int_del_buffer(buffer);
    free(buffer->stream);
    free(buffer);
    return rta;

}

int read_mem(char* registroDatos, char* registroDireccion){
    int dir_fisica = calcular_direccion_fisica(contexto_en_ejecucion->contexto_proceso, registroDireccion);
    if (dir_fisica == -1) {
        actualizar_contexto(fd_memoria);
        sem_wait(&sem_ctx_actualizado);
        segmentation_fault();
        sem_wait(&sem_process_exit);
        return 0; // en realidad hay q hacer lo del segmentation fault
    }

    enviar_read_mem(dir_fisica);
    switch(recibir_operacion(fd_memoria)){
        case READ_MEM_RTA:
            int dato = deserializar_rta_read_mem(fd_memoria);
            *obtenerRegistro(registroDatos) = dato;
            log_trace(logger, "## (<PID:TID>) - (<%d>:<%d>) - Dato Leido: <%i> - Direccion Fisica: <%d>",contexto_en_ejecucion->contexto_hilo->pid,contexto_en_ejecucion->contexto_hilo->tid,dato, dir_fisica);
            contexto_en_ejecucion->contexto_hilo->Registros->PC++;
            break;
        default:
            log_error(logger,"Error desconocido leyendo memoria");
            break;
    }
    return 1;
}

void SUM(char *registro_destino, char *registro_origen){
    uint32_t *destino = obtenerRegistro(registro_destino);
    uint32_t *origen = obtenerRegistro(registro_origen);
    *destino += *origen;
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
}

void SUB(char *registro_destino, char *registro_origen){
    uint32_t *destino = obtenerRegistro(registro_destino);
    uint32_t *origen = obtenerRegistro(registro_origen);

    *destino = *destino - *origen;
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
}

void JNZ(char *registro, uint32_t instruccion){
    uint32_t *reg = obtenerRegistro(registro);

    if(*reg != 0){
        contexto_en_ejecucion->contexto_hilo->Registros->PC = instruccion;
        
    }else{
        contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    }
}

void LOG(char *registro){
    uint32_t *reg = obtenerRegistro(registro);
    log_info(logger, "## LOG - Registro <%s> - Valor <%u>",registro,*reg);
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
}

char *DUMP_MEMORY(int pid, int tid) {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    enviar_dump_memory(pid, tid);
    sem_wait(&sem_dump_mem);
    return rta_mem_dump;
    
    // esperar rta?extern sem_t sem_mutex_lockeado;
}

void io(int tiempo) {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    //contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    enviar_io(tiempo);
    // esperar rta?
        
}

void PROCESS_CREATE(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo){
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    crear_proceso(archivo_de_instrucciones,tamanio_proceso,prio_hilo);
    sem_wait(&sem_proceso_creado);
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
}

void THREAD_CREATE (char* archivo_pseudocodigo, int prioridad) {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    crear_hilo(archivo_pseudocodigo, prioridad);
    sem_wait(&sem_hilo_creado);
    
}

char* THREAD_JOIN (int tid) {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    thread_join(tid);
    sem_wait(&sem_join_hilo);
    return rta_hilo_join;
    
    
    
}

void THREAD_CANCEL (int tid, int pid) {
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    thread_cancel(tid, pid);
    sem_wait(&sem_hilo_cancel);
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
}

void MUTEX_CREATE (char *recurso) {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    mutex_create(recurso);
    sem_wait(&sem_mutex_creado);
}


    

char* MUTEX_LOCK (char* recurso) {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    mutex_lock(recurso);
    sem_wait(&sem_mutex_lockeado);
    return rta_mutex_lock;
}


void MUTEX_UNLOCK (char* recurso) {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    mutex_unlock(recurso);
        
}

void THREAD_EXIT() {
    contexto_en_ejecucion->contexto_hilo->Registros->PC++;
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    thread_exit();
    sem_wait(&sem_thread_exit);
    
    
}

void PROCESS_EXIT() {
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    process_exit();
    sem_wait(&sem_process_exit);
    
}
