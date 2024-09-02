#ifndef UTILIDADES_H_
#define UTILIDADES_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include "utils/utils.h"
#include <commons/collections/list.h>

void iniciar_kernel(void);

int conectarse_a_cpu_interrupt(void);
int conectarse_a_cpu_dispatch(void);
int conectarse_a_memoria(void);
void terminar_ejecucion_a_memoria(void);

#endif //UTILIDADES_H_