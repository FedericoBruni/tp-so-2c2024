#ifndef PLANIFICADOR_CORTO_PLAZO_H
#define PLANIFICADOR_CORTO_PLAZO_H
#include "utilidades.h"
#include "utils/utils.h"
void planificador_corto_plazo();
void fifo();
void prioridad();
int comparar_prioridades(TCB* tcb1, TCB* tcb2);
void multinivel();
void fin_de_quantum(int quantumCola);
#endif