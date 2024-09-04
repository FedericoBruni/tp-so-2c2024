#include "utilidades.h"

t_log *logger;
t_config *config;
char *ip_memoria;
char *puerto_memoria;
char *ip_cpu;
char *puerto_cpu_dispatch;
char *puerto_cpu_interrupt;
char *algoritmo_planificacion;
int *quantum;
char *log_level;
t_list *cola_new;
t_list *cola_ready;
t_list *cola_blocked;
t_list *cola_exit;

void iniciar_kernel(void)
{
    config = iniciar_config("kernel.config");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_cpu = config_get_string_value(config, "IP_CPU");
    puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    quantum = config_get_int_value(config, "QUANTUM");
    log_level = config_get_string_value(config, "LOG_LEVEL");
    logger = iniciar_logger("logKernel.log", "Kernel", log_level_from_string(log_level));
    cola_new = list_create();
    cola_ready = list_create();
    cola_blocked = list_create();
    cola_exit = list_create();
}

int conectarse_a_cpu_interrupt(void)
{
    return crear_conexion(ip_cpu, puerto_cpu_interrupt, logger);
}

int conectarse_a_cpu_dispatch(void)
{
    return crear_conexion(ip_cpu, puerto_cpu_dispatch, logger);
}

int conectarse_a_memoria(void)
{
    return crear_conexion(ip_memoria, puerto_memoria, logger);
}

void terminar_ejecucion(int dispatch, int memoria, int interrupt)
{
    log_info(logger, "Finalizando ejecucion de KERNEL");
    close(dispatch);
    close(memoria);
    close(interrupt);
    log_destroy(logger);
    config_destroy(config);
    list_destroy_and_destroy_elements(cola_new,destruir_pcb);
    list_destroy_and_destroy_elements(cola_ready,destruir_pcb);
    list_destroy_and_destroy_elements(cola_blocked,destruir_pcb);
    list_destroy_and_destroy_elements(cola_exit,destruir_pcb);
}

/* Ejemplo
void person_destroy(void *ptr)
{
    t_person *person = (t_person *)ptr;
    free(person->name);
    free(person);
} */


void destruir_tcb(void* ptr_tcb) {
    TCB *tcb = (TCB*) ptr_tcb;
    //free(tcb->prioridad);
    //free(tcb->tid);
    free(tcb);
}

void destruir_pcb(void* ptr_pcb) {
    PCB* pcb = (PCB*) ptr_pcb;
    list_destroy_and_destroy_elements(pcb->tids,destruir_tcb);
    // funcion para destruir mutex
    //list_destroy(pcb->mutexes);
    // funcion para destruir registros
    //free(pcb->pid);
    free(pcb);
}

int solicitar_memoria(int socket_memoria,int tamanio_memoria, op_code cod_sol){

    t_paquete* paquete = crear_paquete(cod_sol);
    agregar_a_paquete(paquete,"aaaaaa",sizeof("aaaaaa"));
    enviar_paquete(paquete,socket_memoria);
    if(recibir_operacion(socket_memoria) == OK_SOLICITUD_MEMORIA_PROCESO){
        return 1;
    }
    else{
        return 0;
    }
    

}