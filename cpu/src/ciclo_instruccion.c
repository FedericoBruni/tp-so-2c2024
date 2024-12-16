#include "ciclo_instruccion.h"
extern sem_t sem_ejecucion;
extern sem_t sem_hay_interrupt;
extern CONTEXTO_CPU *contexto_en_ejecucion;
extern int fd_memoria;
extern int cliente_fd_dispatch;
extern t_log* logger;
extern bool flag_interrupt;
extern pthread_mutex_t mutex_interrupt;
extern TCB* pcb_en_ejecucion;
extern sem_t sem_fin_q;
extern sem_t sem_interrupt_recibida;
extern sem_t sem_ctx_actualizado;
char* instruccion;
extern bool fin_ciclo;

extern int cliente_fd_interrupt;

void ciclo_de_instruccion() {
    while (true) {
        if(instruccion){
            free(instruccion);
        }
        sem_wait(&sem_ejecucion);
        if(fin_ciclo) return;
        instruccion = fetch();
        if(instruccion){
            char* instruccion_a_ejecutar = decode(instruccion);
            if(instruccion_a_ejecutar != NULL && string_equals_ignore_case(instruccion_a_ejecutar,"SUSPPROCESO")){
                suspender_proceso();
                //free(instruccion);
                continue;
            }
            if(instruccion_a_ejecutar != NULL && string_equals_ignore_case(instruccion_a_ejecutar, "DUMP_ERROR")){
                continue;
            }
        }else{
            enviar_fin_de_proceso();
            continue;
        }
        if (check_interrupt()){ 
            desalojar_por_quantum();
        }
        else sem_post(&sem_ejecucion);   
    }
    
}

// En este momento, se deberá chequear si el Kernel nos envió una interrupción al TID 
// que se está ejecutando, en caso afirmativo, se actualiza el Contexto de Ejecución en 
// la Memoria y se devuelve el TID al Kernel con motivo de la interrupción. 
// Caso contrario, se descarta la interrupción.
bool check_interrupt() {
    pthread_mutex_lock(&mutex_interrupt);
    if(flag_interrupt){
        flag_interrupt = false;
        pthread_mutex_unlock(&mutex_interrupt);
        return true;
    }
    pthread_mutex_unlock(&mutex_interrupt);
    return false;
}

void enviar_fin_de_proceso(){
    int cod_op = FIN_DE_ARCHIVO;
    send(cliente_fd_dispatch, &cod_op,sizeof(cod_op),0);
}

void suspender_proceso(){
    int cod_op = SUSP_PROCESO;
    send(cliente_fd_dispatch, &cod_op,sizeof(cod_op),0);
}

void desalojar_por_quantum(){
    actualizar_contexto(fd_memoria);
    sem_wait(&sem_ctx_actualizado);
    int cod_op = DESALOJO_POR_QUANTUM;
    send(cliente_fd_dispatch, &cod_op,sizeof(cod_op),0);
}

void pedir_prox_instruccion(){
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer,contexto_en_ejecucion->contexto_hilo->Registros->PC);
    cargar_int_al_buffer(buffer,contexto_en_ejecucion->contexto_hilo->pid );
    cargar_int_al_buffer(buffer,contexto_en_ejecucion->contexto_hilo->tid);
    t_paquete *paquete = crear_paquete(SOLICITAR_INSTRUCCION, buffer);
    enviar_paquete(paquete, fd_memoria);
    eliminar_paquete(paquete);
    sem_post(&sem_fin_q);
}

char* recibir_prox_instruccion(){
    int cod_op = recibir_operacion(fd_memoria);
    switch(cod_op){
        case PROXIMA_INSTRUCCION:
            t_buffer* buffer = recibir_buffer_completo(fd_memoria);
            char* rta = extraer_string_del_buffer(buffer);
            free(buffer->stream);
            free(buffer);
            return rta;
        case EOF_INSTRUCCION:
            return NULL;
        default:
            log_error(logger, "Error al intentar obtener la instruccion - PC: <%d>",contexto_en_ejecucion->contexto_hilo->Registros->PC);
            break;
    }
}

char* fetch() {
	log_info(logger, "## TID: <%d> - FETCH - Program Counter: <%d>", contexto_en_ejecucion->contexto_hilo->tid, contexto_en_ejecucion->contexto_hilo->Registros->PC);
    pedir_prox_instruccion();
    char* instruccion = recibir_prox_instruccion();    
    return instruccion;
}

char* corregir_linea(char* string) {
    if(string[strlen(string)-1] == '\n')
            string[strlen(string)-1] = '\0';
    return string;
}

char* decode(char* instruccion) {
    if(!instruccion) return NULL;
    char** lista = string_split(instruccion, " ");
    char* instr = lista[0];
    if (string_equals_ignore_case(corregir_linea(instr), "SET")){
        char* registro = lista[1];
        int valor = atoi(corregir_linea(lista[2]));
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s %d>", contexto_en_ejecucion->contexto_hilo->tid, instr,registro,valor);
        SET(registro,valor);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "READ_MEM")){
        char* registroDatos = lista[1];
        char* registroDireccion = corregir_linea(lista[2]);
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s %s>", contexto_en_ejecucion->contexto_hilo->tid, instr,registroDatos,registroDireccion);
        read_mem(registroDatos, registroDireccion);
        string_array_destroy(lista);
        return "OK";
        
    } else if (string_equals_ignore_case(corregir_linea(instr), "WRITE_MEM")){
        char* registroDatos = lista[1];
        char* registroDireccion = corregir_linea(lista[2]);
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s %s>", contexto_en_ejecucion->contexto_hilo->tid, instr,registroDatos,registroDireccion);
        if (write_mem(registroDatos, registroDireccion)) {
            string_array_destroy(lista);
            return "OK";
        }
        string_array_destroy(lista);
        return "SUSPPROCESO";
        
    } else if (string_equals_ignore_case(corregir_linea(instr), "SUM")){
        char* registroDestino = lista[1];
        printf("destino: %s\n",registroDestino);
        char* registroOrigen = corregir_linea(lista[2]);
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s %s>", contexto_en_ejecucion->contexto_hilo->tid, instr,registroDestino,registroOrigen);
        SUM(registroDestino,registroOrigen);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "SUB")){
        char* registroDestino = lista[1];
        char* registroOrigen = corregir_linea(lista[2]);
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s %s>", contexto_en_ejecucion->contexto_hilo->tid, instr,registroDestino,registroOrigen);
        SUB(registroDestino,registroOrigen);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "JNZ")){
        char* registro = lista[1];
        int valor = atoi(corregir_linea(lista[2]));
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s %d>", contexto_en_ejecucion->contexto_hilo->tid, instr,registro,valor);
        JNZ(registro,valor);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "LOG")){
        char* registro = corregir_linea(lista[1]);
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s>", contexto_en_ejecucion->contexto_hilo->tid, instr,registro);
        LOG(registro);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "DUMP_MEMORY")){
        log_info(logger, "## TID: <%d> - Ejecutando: <%s>", contexto_en_ejecucion->contexto_hilo->tid, instr);
        char *rta =DUMP_MEMORY(contexto_en_ejecucion->contexto_hilo->pid, contexto_en_ejecucion->contexto_hilo->tid);
        string_array_destroy(lista);
        
        return rta;
        
    } else if (string_equals_ignore_case(corregir_linea(instr), "IO")){
        int milisegundos = atoi(lista[1]);
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%d>", contexto_en_ejecucion->contexto_hilo->tid, instr,milisegundos);
        io(milisegundos);
        string_array_destroy(lista);
        return "SUSPPROCESO";
    } else if (string_equals_ignore_case(corregir_linea(instr), "PROCESS_CREATE")){
        char* archivo = lista[1];
        int tam = atoi(lista[2]);
        int prio = atoi(lista[3]);
        log_info(logger, "## TID: <%d> - Ejecutando: <%s> - <%s %d %d>", contexto_en_ejecucion->contexto_hilo->tid, instr,archivo,tam,prio);
        PROCESS_CREATE(archivo,tam,prio);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "THREAD_CREATE")){
        char* archivo = lista[1];
        int prio = atoi(lista[2]);
        log_info(logger, "## TID: <%d> - Ejecutando: <THREAD_CREATE> - <%s %d>", contexto_en_ejecucion->contexto_hilo->tid, archivo, prio);
        THREAD_CREATE(archivo,prio);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "THREAD_JOIN")){
        int tid = atoi(lista[1]);
        log_info(logger, "## TID: <%d> - Ejecutando: <THREAD_JOIN> - <%d>", contexto_en_ejecucion->contexto_hilo->tid, tid);
        char *rta = THREAD_JOIN(tid);
        string_array_destroy(lista);
        return rta;
    } else if (string_equals_ignore_case(corregir_linea(instr), "THREAD_CANCEL")){
        int tid = atoi(lista[1]);
        log_info(logger, "## TID: <%d> - Ejecutando: <THREAD_CANCEL> - <%d>", contexto_en_ejecucion->contexto_hilo->tid, tid);
        THREAD_CANCEL(tid, contexto_en_ejecucion->contexto_hilo->pid);
        if(tid == contexto_en_ejecucion->contexto_hilo->tid){
            string_array_destroy(lista);
            return "SUSPPROCESO";
        }
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "MUTEX_CREATE")){
        char* recurso = corregir_linea(lista[1]);
        log_info(logger, "## TID: <%d> - Ejecutando: <MUTEX_CREATE> - <%s>", contexto_en_ejecucion->contexto_hilo->tid, recurso);
        MUTEX_CREATE(recurso);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "MUTEX_LOCK")){
        char* recurso = corregir_linea(lista[1]);
        log_info(logger, "## TID: <%d> - Ejecutando: <MUTEX_LOCK> - <%s>", contexto_en_ejecucion->contexto_hilo->tid, recurso);
        char* rta = MUTEX_LOCK(recurso);
        string_array_destroy(lista);
        return rta;
    } else if (string_equals_ignore_case(corregir_linea(instr), "MUTEX_UNLOCK")){
        char* recurso = corregir_linea(lista[1]);
        log_info(logger, "## TID: <%d> - Ejecutando: <MUTEX_UNLOCK> - <%s>", contexto_en_ejecucion->contexto_hilo->tid, recurso);
        MUTEX_UNLOCK(recurso);
        string_array_destroy(lista);
        return "OK";
    } else if (string_equals_ignore_case(corregir_linea(instr), "THREAD_EXIT")){
        log_info(logger, "## TID: <%d> - Ejecutando: <THREAD_EXIT>", contexto_en_ejecucion->contexto_hilo->tid);
        string_array_destroy(lista);
        THREAD_EXIT();
        return "SUSPPROCESO";
    } else if (string_equals_ignore_case(corregir_linea(instr), "PROCESS_EXIT")){
        log_info(logger, "## TID: <%d> - Ejecutando: <PROCESS_EXIT>", contexto_en_ejecucion->contexto_hilo->tid);
        PROCESS_EXIT();
        string_array_destroy(lista);
        return "SUSPPROCESO";
    }
    imprimir_ctx_cpu(contexto_en_ejecucion);
    //sem_post(&sem_ejecucion);
    //printf("POST\n");
}

void imprimir_ctx_cpu(CONTEXTO_CPU *contexto_cpu) {
    
    printf("CTX de <PID: %i>:<TID: %i>\n", contexto_cpu->contexto_hilo->pid, contexto_cpu->contexto_hilo->tid);
    printf("REGISTROS: PC: %i, AX: %i, BX: %i, CX: %i, DX: %i\n", contexto_cpu->contexto_hilo->Registros->PC, contexto_cpu->contexto_hilo->Registros->AX,contexto_cpu->contexto_hilo->Registros->BX ,contexto_cpu->contexto_hilo->Registros->CX, contexto_cpu->contexto_hilo->Registros->DX);
    printf("REGISTROS: EX: %i, FX: %i, GX: %i, HX: %i, BASE: %i, LIMITE: %i\n", contexto_cpu->contexto_hilo->Registros->EX, contexto_cpu->contexto_hilo->Registros->FX, contexto_cpu->contexto_hilo->Registros->GX, contexto_cpu->contexto_hilo->Registros->HX, contexto_cpu->contexto_proceso->BASE, contexto_cpu->contexto_proceso->LIMITE);
    printf("--------------------------------------------------\n");
}