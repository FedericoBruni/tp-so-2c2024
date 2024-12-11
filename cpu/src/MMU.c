#include "MMU.h"
extern t_log *logger;

int calcular_direccion_fisica(CONTEXTO_PROCESO* ctx_proceso, char* desplazamiento){
    uint32_t offset = *obtenerRegistro(desplazamiento);
    int rta = ctx_proceso->BASE + offset;
    log_trace(logger,"RESPUESTA: %i BASE: %i LIMITE: %i",rta,ctx_proceso->BASE,ctx_proceso->LIMITE);
    if (rta > ctx_proceso->LIMITE){
        log_error(logger, "ERROR, offset (%i) > limite (%i)", offset, ctx_proceso->LIMITE);
        return -1;
    }
    return rta;
}