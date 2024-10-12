#include "instrucciones.h"
extern CONTEXTO_CPU *contexto_en_ejecucion;
extern t_log *logger;

void SET(char *registro, uint32_t valor){
    uint32_t *reg = obtenerRegistro(registro);

    *reg = valor;
}

// void READ_MEM(){

// }

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

PROCESS_CREATE(char *archivo_de_instrucciones,int tamanio_proceso, int prio_hilo){
    //actualizar contexto()
    
}