#ifndef CICLO_INSTRUCCION_H
#define CICLO_INSTRUCCION_H
#include "utils/utils.h"
#include "main.h"
#include "instrucciones.h"
#include <commons/string.h>

void ciclo_de_instruccion();
char* fetch();
char* decode(char* instruccion);
void pedir_prox_instruccion();
char* recibir_prox_instruccion();
void imprimir_ctx_cpu(CONTEXTO_CPU *contexto_cpu); //de prueba
void suspender_proceso();
void desalojar_por_quantum();
bool check_interrupt();
#endif /* CICLO_INSTRUCCION_H */