#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H
#include "utils/utils.h"
#include "main.h"

void SET(char *registro, uint32_t valor);
void SUM(char *registro_destino, char *registro_origen);
void SUB(char *registro_destino, char *registro_origen);
void JNZ(char *registro, uint32_t instruccion);
void LOG(char *registro);

#endif /*INSTRUCCIONES_H*/