#include "ciclo_instruccion.h"
extern sem_t sem_ejecucion;
extern CONTEXTO_CPU *contexto_en_ejecucion;
extern int fd_memoria;
extern int cliente_fd_dispatch;
extern t_log* logger;

void ciclo_de_instruccion() {
    while (true) {
        sem_wait(&sem_ejecucion);
        char* instruccion = fetch();
        printf("instr: %s\n",instruccion);
        if(instruccion){
            printf("ENTRO A DECODE\n");
            char* instruccion_a_ejecutar = decode(instruccion);
            if(instruccion_a_ejecutar != NULL &&string_equals_ignore_case(instruccion_a_ejecutar,"SUSPPROCESO")){
                suspender_proceso();
            }else{sem_post(&sem_ejecucion);}
        }else{
            printf("ENTRO A FIN DE ARCHIVO\n");
            enviar_fin_de_proceso();
        }

        //execute();

        //check_interrupt();
        
    }
}

void enviar_fin_de_proceso(){
    int cod_op = FIN_DE_ARCHIVO;
    send(cliente_fd_dispatch, &cod_op,sizeof(cod_op),0);
}
void suspender_proceso(){
    int cod_op = SUSP_PROCESO;
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
}

char* recibir_prox_instruccion(){
    int cod_op = recibir_operacion(fd_memoria);
    switch(cod_op){
        case PROXIMA_INSTRUCCION:
            t_buffer* buffer = recibir_buffer_completo(fd_memoria);
            return extraer_string_del_buffer(buffer);
            break;
        case EOF_INSTRUCCION:
            log_info(logger,"Fin del archivo");
            return NULL;
        default:
            log_error(logger, "Error al intentar obtener la instruccion: %d",contexto_en_ejecucion->contexto_hilo->Registros->PC);
            break;
    }
}

char* fetch() {
    pedir_prox_instruccion();
    char* instruccion = recibir_prox_instruccion();
    printf("instr dentro de feth: %s\n",instruccion);
    if(instruccion){
        printf("Instruccion recibida: %s\n",instruccion);
        contexto_en_ejecucion->contexto_hilo->Registros->PC++;
        
    }
    
    
    return instruccion;
}

char* decode(char* instruccion) {
    if(!instruccion) return NULL;
    char** lista = string_split(instruccion, " ");
    char* instr = lista[0];
    if (string_equals_ignore_case(instr, "SET")){
        char* registro = lista[1];
        int valor = atoi(lista[2]);
        SET(registro,valor);
        return "OK";
    } else if (string_equals_ignore_case(instr, "READ_MEM")){
        char* registroDatos = lista[1];
        char* registroDireccion = lista[2];
        registroDireccion[strlen(registroDireccion)-1] = '\0';
        READ_MEM(registroDatos, registroDireccion);
        return "OK";
        
    } else if (string_equals_ignore_case(instr, "WRITE_MEM")){
        
    } else if (string_equals_ignore_case(instr, "SUM")){
        char* registroDestino = lista[1];
        printf("destino: %s\n",registroDestino);
        char* registroOrigen = lista[2];
        registroOrigen[strlen(registroOrigen)-1] = '\0';
        printf("origen: %s\n",registroOrigen);
        SUM(registroDestino,registroOrigen);
        return "OK";
    } else if (string_equals_ignore_case(instr, "SUB")){
        char* registroDestino = lista[1];
        printf("destino: %s\n",registroDestino);
        char* registroOrigen = lista[2];
        registroOrigen[strlen(registroOrigen)-1] = '\0';
        printf("origen: %s\n",registroOrigen);
        SUB(registroDestino,registroOrigen);
        return "OK";
    } else if (string_equals_ignore_case(instr, "JNZ")){
        char* registro = lista[1];
        int valor = atoi(lista[2]);
        JNZ(registro,valor);
        return "OK";
    } else if (string_equals_ignore_case(instr, "LOG")){
        char* registro = lista[1];
        registro[strlen(registro)-1] = '\0';
        LOG(registro);
        return "OK";
    } else if (string_equals_ignore_case(instr, "DUMP_MEMORY")){
        DUMP_MEMORY(contexto_en_ejecucion->contexto_hilo->pid, contexto_en_ejecucion->contexto_hilo->tid);
        return "OK";
        
    } else if (string_equals_ignore_case(instr, "IO")){
        char* milisegundos = lista[1];
        milisegundos[strlen(milisegundos)-1] = '\0';
        IO(atoi(milisegundos));
        return "OK";
        
    } else if (string_equals_ignore_case(instr, "PROCESS_CREATE")){
        char* archivo = lista[1];
        int tam = atoi(lista[2]);
        int prio = atoi(lista[3]);
        PROCESS_CREATE(archivo,tam,prio);
        return "OK";
    } else if (string_equals_ignore_case(instr, "THREAD_CREATE")){
        char* archivo = lista[1];
        int prio = atoi(lista[2]);
        THREAD_CREATE(archivo,prio);
        return "OK";
    } else if (string_equals_ignore_case(instr, "THREAD_JOIN")){
        int tid = atoi(lista[1]);
        THREAD_JOIN(tid);
        return "SUSPPROCESO";
    } else if (string_equals_ignore_case(instr, "THREAD_CANCEL")){
        
    } else if (string_equals_ignore_case(instr, "MUTEX_CREATE")){
        
    } else if (string_equals_ignore_case(instr, "MUTEX_LOCK")){
        
    } else if (string_equals_ignore_case(instr, "MUTEX_UNLOCK")){
        
    } else if (string_equals_ignore_case(instr, "THREAD_EXIT")){
        
    } else if (string_equals_ignore_case(instr, "PROCESS_EXIT")){
        
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