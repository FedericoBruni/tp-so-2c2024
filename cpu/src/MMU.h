#ifndef MMU_H
#define MMU_H
#include "utilidades.h"
#include "utils/utils.h"

int calcular_direccion_fisica(CONTEXTO_PROCESO* ctx_proceso, char* desplazamiento);

#endif